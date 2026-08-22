/**
 * Chessground renders pieces as a custom `<piece>` element and styles them by
 * role/colour class. The promotion picker reuses those sprites.
 */
import "react";

declare module "react" {
  namespace JSX {
    interface IntrinsicElements {
      piece: React.DetailedHTMLProps<React.HTMLAttributes<HTMLElement>, HTMLElement>;
    }
  }
}
