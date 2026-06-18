import { serializeLevel } from "../lib/level-io";
import type { LevelData } from "../types/level";

interface Props {
  level: LevelData;
}

export function JsonPreview({ level }: Props) {
  return (
    <section className="json-preview">
      <h2>JSON preview</h2>
      <pre>{serializeLevel(level)}</pre>
    </section>
  );
}
