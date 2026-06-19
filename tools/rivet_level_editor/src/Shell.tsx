import { useState } from "react";
import { App } from "./App";
import { ModelEditor } from "./ModelEditor";

export type WorkspaceId = "level" | "model";

export function Shell() {
  const [workspace, setWorkspace] = useState<WorkspaceId>("level");

  return (
    <div className="workspace-shell">
      <nav className="workspace-tabs" aria-label="Editor workspace">
        <button
          type="button"
          className={workspace === "level" ? "active" : ""}
          onClick={() => setWorkspace("level")}
        >
          Level Editor
        </button>
        <button
          type="button"
          className={workspace === "model" ? "active" : ""}
          onClick={() => setWorkspace("model")}
        >
          Model Editor
        </button>
      </nav>
      <div className="workspace-body">
        {workspace === "level" ? <App /> : <ModelEditor />}
      </div>
    </div>
  );
}
