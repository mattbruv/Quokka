import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import {
  Badge,
  Box,
  Button,
  Card,
  Flex,
  Heading,
  HStack,
  IconButton,
  Input,
  SegmentGroup,
  Separator,
  Slider,
  Spinner,
  Stack,
  Text,
} from "@chakra-ui/react";

import { LuGithub } from "react-icons/lu";

import { Chessground } from "@lichess-org/chessground";
import type { Api } from "@lichess-org/chessground/api";
import type { Color, Key } from "@lichess-org/chessground/types";
import { Chess, DEFAULT_POSITION } from "chess.js";
import type { Square } from "chess.js";

import "@lichess-org/chessground/assets/chessground.base.css";
import "./css/chessground.brown.css";
import "./css/chessground.cburnett.css";
import "./css/app.css";

import { ColorModeButton } from "./src/components/ui/color-mode.tsx";
import { Engine, isMateScore } from "./engine/engine.ts";
import type { EngineInfo } from "./engine/engine.ts";
import {
  PROMOTION_CHOICES,
  checkedColor,
  formatNodes,
  formatScore,
  gameStatus,
  lastMoveKeys,
  parseUci,
  toColor,
  toDests,
  toMovePairs,
} from "./util.ts";
import type { PromotionSymbol } from "./util.ts";

const REPO_URL = "https://github.com/mattbruv/Quokka";

const MIN_DEPTH = 1;
const MAX_DEPTH = 7;

interface PendingPromotion {
  from: Square;
  to: Square;
  color: Color;
}

interface Analysis {
  info: EngineInfo;
  /** Principal variation rendered in SAN, from the searched position. */
  pv: string[];
  /** Score normalised to White's point of view. */
  whiteScore: number;
}

/** Square of a promotion choice, as a percentage offset inside the board. */
function choicePosition(dest: Square, index: number, orientation: Color) {
  const file = dest.charCodeAt(0) - "a".charCodeAt(0);
  const rank = Number(dest[1]) - 1;
  const column = orientation === "white" ? file : 7 - file;
  const rowFromTop = orientation === "white" ? 7 - rank : rank;
  // The picker hangs off the promotion square, away from the board edge.
  const row = rowFromTop === 0 ? index : rowFromTop - index;
  return { left: `${column * 12.5}%`, top: `${row * 12.5}%` };
}

function pvToSan(fen: string, pv: string[]): string[] {
  const board = new Chess(fen);
  const san: string[] = [];
  for (const uci of pv) {
    const move = parseUci(uci);
    if (!move) break;
    try {
      san.push(board.move(move).san);
    } catch {
      break;
    }
  }
  return san;
}

function App() {
  const engine = useMemo(() => new Engine(), []);
  const chessRef = useRef(new Chess());
  const boardRef = useRef<HTMLDivElement>(null);
  const apiRef = useRef<Api | null>(null);
  const depthRef = useRef(4);

  // Bumped on every mutation of `chessRef`, since the Chess object is mutable.
  const [version, setVersion] = useState(0);
  const [playerColor, setPlayerColor] = useState<Color>("white");
  const [orientation, setOrientation] = useState<Color>("white");
  const [depth, setDepth] = useState(4);
  const [thinking, setThinking] = useState(false);
  const [analysis, setAnalysis] = useState<Analysis | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [promotion, setPromotion] = useState<PendingPromotion | null>(null);
  const [fenInput, setFenInput] = useState(DEFAULT_POSITION);
  // While the user is typing a FEN, don't overwrite it with the live position.
  const [fenEdited, setFenEdited] = useState(false);
  const [copied, setCopied] = useState<string | null>(null);

  const bump = useCallback(() => setVersion((v) => v + 1), []);

  const chess = chessRef.current;
  const fen = chess.fen();
  const status = gameStatus(chess);
  const movePairs = toMovePairs(chess);
  const turn = toColor(chess);

  depthRef.current = depth;

  const onUserMove = useCallback(
    (orig: Key, dest: Key) => {
      const board = chessRef.current;
      const from = orig as Square;
      const to = dest as Square;
      const legal = board
        .moves({ square: from, verbose: true })
        .filter((m) => m.to === to);
      if (!legal.length) return;

      if (legal.some((m) => m.promotion)) {
        // Leave the pawn where chessground dropped it and ask which piece.
        setPromotion({ from, to, color: toColor(board) });
        return;
      }
      board.move({ from, to });
      setError(null);
      bump();
    },
    [bump],
  );

  // Chessground is imperative, so it is created once and kept in a ref.
  const onUserMoveRef = useRef(onUserMove);
  onUserMoveRef.current = onUserMove;

  useEffect(() => {
    if (!boardRef.current) return;
    const api = Chessground(boardRef.current, {
      fen: chessRef.current.fen(),
      orientation: "white",
      turnColor: "white",
      highlight: { lastMove: true, check: true },
      animation: { enabled: true, duration: 200 },
      premovable: { enabled: false },
      draggable: { showGhost: true },
      movable: {
        free: false,
        color: "white",
        dests: toDests(chessRef.current),
        events: { after: (orig, dest) => onUserMoveRef.current(orig, dest) },
      },
    });
    apiRef.current = api;
    return () => {
      api.destroy();
      apiRef.current = null;
    };
  }, []);

  // Push the current game state onto the board.
  useEffect(() => {
    const api = apiRef.current;
    if (!api) return;

    if (promotion) {
      // Freeze the board under the picker; the pawn is already on its square.
      api.set({ movable: { dests: new Map() } });
      return;
    }

    const board = chessRef.current;
    const over = gameStatus(board).over;
    api.set({
      fen: board.fen(),
      orientation,
      turnColor: toColor(board),
      check: checkedColor(board) ?? false,
      lastMove: lastMoveKeys(board),
      movable: {
        color: playerColor,
        dests: over ? new Map() : toDests(board),
      },
    });
  }, [version, orientation, playerColor, promotion]);

  // Let the engine move whenever it is on turn.
  useEffect(() => {
    const board = chessRef.current;
    if (promotion || gameStatus(board).over || toColor(board) === playerColor) {
      setThinking(false);
      return;
    }

    let cancelled = false;
    const searchFen = board.fen();
    setThinking(true);

    engine
      .search(searchFen, depthRef.current)
      .then((result) => {
        if (cancelled) return;
        setThinking(false);

        if (result.info) {
          const whiteScore =
            searchFen.split(" ")[1] === "b"
              ? -result.info.score
              : result.info.score;
          setAnalysis({
            info: result.info,
            pv: pvToSan(searchFen, result.info.pv),
            whiteScore,
          });
        }

        const move = result.bestMove ? parseUci(result.bestMove) : null;
        if (!move) {
          setError("The engine did not return a move for this position.");
          return;
        }
        try {
          board.move(move);
        } catch {
          setError(`The engine returned an illegal move (${result.bestMove}).`);
          return;
        }
        setError(null);
        bump();
      })
      .catch((err: Error) => {
        if (cancelled || err.message === "aborted") return;
        setThinking(false);
        setError(`Engine error: ${err.message}`);
      });

    return () => {
      cancelled = true;
    };
  }, [version, playerColor, promotion, engine, bump]);

  useEffect(() => {
    void engine.preload().catch(() => undefined);
    return () => engine.destroy();
  }, [engine]);

  useEffect(() => {
    if (!fenEdited) setFenInput(fen);
  }, [fen, fenEdited]);

  useEffect(() => {
    if (!copied) return;
    const timer = setTimeout(() => setCopied(null), 1500);
    return () => clearTimeout(timer);
  }, [copied]);

  function resetTo(startFen: string | undefined, color: Color) {
    engine.restart();
    try {
      chessRef.current = new Chess(startFen);
    } catch {
      setError("That FEN is not a valid position.");
      return;
    }
    setPromotion(null);
    setAnalysis(null);
    setError(null);
    setThinking(false);
    setPlayerColor(color);
    setOrientation(color);
    setFenEdited(false);
    bump();
  }

  function choosePromotion(symbol: PromotionSymbol) {
    if (!promotion) return;
    chessRef.current.move({
      from: promotion.from,
      to: promotion.to,
      promotion: symbol,
    });
    setPromotion(null);
    setError(null);
    bump();
  }

  function cancelPromotion() {
    // Clearing the pending promotion re-syncs the board, which puts the pawn back.
    setPromotion(null);
  }

  function switchSides(color: Color) {
    engine.restart();
    setPromotion(null);
    setPlayerColor(color);
    setOrientation(color);
  }

  function takeBack() {
    const board = chessRef.current;
    if (!board.history().length) return;
    engine.restart();
    board.undo();
    // Stop on the player's own turn, otherwise the engine just replays.
    if (board.history().length && toColor(board) !== playerColor) board.undo();
    setPromotion(null);
    setAnalysis(null);
    setError(null);
    setThinking(false);
    bump();
  }

  async function copy(label: string, value: string) {
    try {
      await navigator.clipboard.writeText(value);
      setCopied(label);
    } catch {
      setError("Could not write to the clipboard.");
    }
  }

  const canTakeBack = chess.history().length > 0 && !promotion;
  const engineColor: Color = playerColor === "white" ? "black" : "white";

  return (
    <Flex
      direction={{ base: "column", lg: "row" }}
      gap={6}
      p={{ base: 4, md: 6 }}
      maxW="1200px"
      mx="auto"
      align="flex-start"
    >
      <Box flex="1 1 420px" minW={0} w="100%" maxW="min(72vh, 620px)">
        <div className="board-stack">
          <div ref={boardRef} style={{ width: "100%", height: "100%" }} />
          {promotion && (
            <div
              className="cg-wrap promotion-overlay"
              role="dialog"
              aria-label="Choose a promotion piece"
              onClick={cancelPromotion}
              onContextMenu={(e) => {
                e.preventDefault();
                cancelPromotion();
              }}
            >
              {PROMOTION_CHOICES.map((choice, index) => (
                <button
                  key={choice.role}
                  type="button"
                  className="promotion-choice"
                  aria-label={`Promote to ${choice.role}`}
                  style={choicePosition(promotion.to, index, orientation)}
                  onClick={(e) => {
                    e.stopPropagation();
                    choosePromotion(choice.symbol);
                  }}
                >
                  <piece className={`${choice.role} ${promotion.color}`} />
                </button>
              ))}
            </div>
          )}
        </div>

        <HStack mt={3} justify="space-between" wrap="wrap" gap={2}>
          <HStack gap={2}>
            <Badge
              colorPalette={
                status.over ? "purple" : turn === "white" ? "gray" : "blue"
              }
            >
              {status.label}
            </Badge>
            {thinking && (
              <HStack gap={2} color="fg.muted">
                <Spinner size="xs" />
                <Text fontSize="sm">Quokka is thinking…</Text>
              </HStack>
            )}
          </HStack>
          <Text fontSize="sm" color="fg.muted">
            You play {playerColor} · Quokka plays {engineColor}
          </Text>
        </HStack>

        {promotion && (
          <Text mt={2} fontSize="sm" color="fg.muted">
            Pick a piece to promote to, or click the board to cancel.
          </Text>
        )}
        {error && (
          <Text mt={2} fontSize="sm" color="red.500">
            {error}
          </Text>
        )}
      </Box>

      <Stack flex="1 1 320px" minW={0} w="100%" gap={5}>
        <HStack justify="space-between" align="flex-start">
          <Box>
            <Heading size="lg">Quokka</Heading>
            <Text color="fg.muted" fontSize="sm">
              A hobby chess engine made in C++ in 2014, compiled to WebAssembly.
            </Text>
          </Box>
          <HStack gap={0}>
            <IconButton
              asChild
              variant="ghost"
              size="sm"
              aria-label="View the source on GitHub"
              css={{ _icon: { width: "5", height: "5" } }}
            >
              <a href={REPO_URL} target="_blank" rel="noreferrer noopener">
                <LuGithub />
              </a>
            </IconButton>
            <ColorModeButton />
          </HStack>
        </HStack>

        <Stack gap={3}>
          <Box>
            <Text fontSize="sm" fontWeight="medium" mb={1}>
              Play as
            </Text>
            <SegmentGroup.Root
              size="sm"
              value={playerColor}
              onValueChange={(e) => e.value && switchSides(e.value as Color)}
            >
              <SegmentGroup.Indicator />
              <SegmentGroup.Items
                items={[
                  { value: "white", label: "White" },
                  { value: "black", label: "Black" },
                ]}
              />
            </SegmentGroup.Root>
          </Box>

          <Box>
            <HStack justify="space-between" mb={1}>
              <Text fontSize="sm" fontWeight="medium">
                Search depth
              </Text>
              <Text fontSize="sm" color="fg.muted">
                {depth}
              </Text>
            </HStack>
            <Slider.Root
              size="sm"
              min={MIN_DEPTH}
              max={MAX_DEPTH}
              step={1}
              value={[depth]}
              onValueChange={(e) => setDepth(e.value[0])}
            >
              <Slider.Control>
                <Slider.Track>
                  <Slider.Range />
                </Slider.Track>
                <Slider.Thumbs />
              </Slider.Control>
            </Slider.Root>
            <Text fontSize="xs" color="fg.muted" mt={1}>
              Higher depth plays better but takes longer. Applies to the next
              move.
            </Text>
          </Box>

          <HStack wrap="wrap" gap={2}>
            <Button size="sm" onClick={() => resetTo(undefined, playerColor)}>
              New game
            </Button>
            <Button
              size="sm"
              variant="outline"
              onClick={takeBack}
              disabled={!canTakeBack}
            >
              Take back
            </Button>
            <Button
              size="sm"
              variant="outline"
              onClick={() =>
                setOrientation(orientation === "white" ? "black" : "white")
              }
            >
              Flip board
            </Button>
          </HStack>
        </Stack>

        <Separator />

        <Card.Root size="sm" variant="subtle">
          <Card.Body>
            <Text fontSize="sm" fontWeight="medium" mb={2}>
              Engine
            </Text>
            {analysis ? (
              <Stack gap={1} opacity={thinking ? 0.5 : 1}>
                <HStack gap={4} wrap="wrap" fontSize="sm">
                  <Text>
                    Eval{" "}
                    <Text as="span" fontWeight="bold">
                      {formatScore(
                        analysis.whiteScore,
                        isMateScore(analysis.whiteScore),
                        analysis.info.pv.length,
                      )}
                    </Text>
                  </Text>
                  <Text color="fg.muted">depth {analysis.info.depth}</Text>
                  <Text color="fg.muted">
                    {formatNodes(analysis.info.nodes)} nodes
                  </Text>
                  <Text color="fg.muted">{analysis.info.time} ms</Text>
                </HStack>
                {analysis.pv.length > 0 && (
                  <Text className="pv-line">PV: {analysis.pv.join(" ")}</Text>
                )}
              </Stack>
            ) : (
              <Text fontSize="sm" color="fg.muted">
                {thinking ? "Searching…" : "No search yet."}
              </Text>
            )}
          </Card.Body>
        </Card.Root>

        <Box>
          <Text fontSize="sm" fontWeight="medium" mb={2}>
            Moves
          </Text>
          {movePairs.length ? (
            <Box maxH="220px" overflowY="auto" pr={2}>
              <div className="move-list">
                {movePairs.map((pair) => (
                  <Box key={pair.number} display="contents">
                    <span className="move-number">{pair.number}.</span>
                    <span className="move-san">{pair.white ?? "…"}</span>
                    <span className="move-san">{pair.black ?? ""}</span>
                  </Box>
                ))}
              </div>
            </Box>
          ) : (
            <Text fontSize="sm" color="fg.muted">
              No moves yet.
            </Text>
          )}
          {status.result && (
            <Text mt={2} fontSize="sm" fontWeight="bold">
              {status.result}
            </Text>
          )}
        </Box>

        <Separator />

        <Stack gap={2}>
          <Text fontSize="sm" fontWeight="medium">
            Position
          </Text>
          <Input
            size="sm"
            fontFamily="mono"
            fontSize="xs"
            value={fenInput}
            onChange={(e) => {
              setFenInput(e.target.value);
              setFenEdited(true);
            }}
            placeholder="Paste a FEN to set up a position"
          />
          <HStack wrap="wrap" gap={2}>
            <Button
              size="xs"
              variant="outline"
              onClick={() => resetTo(fenInput.trim(), playerColor)}
            >
              Load FEN
            </Button>
            <Button
              size="xs"
              variant="outline"
              onClick={() => copy("FEN", fen)}
            >
              {copied === "FEN" ? "Copied!" : "Copy FEN"}
            </Button>
            <Button
              size="xs"
              variant="outline"
              onClick={() => copy("PGN", chess.pgn())}
            >
              {copied === "PGN" ? "Copied!" : "Copy PGN"}
            </Button>
          </HStack>
        </Stack>
      </Stack>
    </Flex>
  );
}

export default App;
