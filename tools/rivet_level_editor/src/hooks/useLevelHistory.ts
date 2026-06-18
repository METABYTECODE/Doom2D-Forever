import { useCallback, useReducer } from "react";
import { ensureLevelCollision } from "../lib/level-collision";
import type { LevelData } from "../types/level";

const MAX_HISTORY = 60;

type State = {
  level: LevelData;
  past: LevelData[];
  future: LevelData[];
  dirty: boolean;
};

type Action =
  | { type: "replace"; level: LevelData; clean?: boolean }
  | { type: "update"; updater: (prev: LevelData) => LevelData }
  | { type: "undo" }
  | { type: "redo" }
  | { type: "saved" };

function reducer(state: State, action: Action): State {
  switch (action.type) {
    case "replace":
      return {
        level: ensureLevelCollision(action.level),
        past: [],
        future: [],
        dirty: !action.clean,
      };
    case "update": {
      const next = ensureLevelCollision(action.updater(state.level));
      if (next === state.level) return state;
      return {
        level: next,
        past: [...state.past.slice(-MAX_HISTORY + 1), state.level],
        future: [],
        dirty: true,
      };
    }
    case "undo": {
      if (state.past.length === 0) return state;
      const previous = state.past[state.past.length - 1];
      return {
        level: previous,
        past: state.past.slice(0, -1),
        future: [state.level, ...state.future],
        dirty: true,
      };
    }
    case "redo": {
      if (state.future.length === 0) return state;
      const next = state.future[0];
      return {
        level: next,
        past: [...state.past, state.level],
        future: state.future.slice(1),
        dirty: true,
      };
    }
    case "saved":
      return { ...state, dirty: false };
    default:
      return state;
  }
}

export function useLevelHistory(initial: LevelData) {
  const [state, dispatch] = useReducer(reducer, {
    level: initial,
    past: [],
    future: [],
    dirty: false,
  });

  const replace = useCallback((level: LevelData, clean = false) => {
    dispatch({ type: "replace", level, clean });
  }, []);

  const update = useCallback((updater: (prev: LevelData) => LevelData) => {
    dispatch({ type: "update", updater });
  }, []);

  const undo = useCallback(() => dispatch({ type: "undo" }), []);
  const redo = useCallback(() => dispatch({ type: "redo" }), []);
  const markSaved = useCallback(() => dispatch({ type: "saved" }), []);

  return {
    level: state.level,
    dirty: state.dirty,
    replace,
    update,
    undo,
    redo,
    markSaved,
    canUndo: state.past.length > 0,
    canRedo: state.future.length > 0,
  };
}
