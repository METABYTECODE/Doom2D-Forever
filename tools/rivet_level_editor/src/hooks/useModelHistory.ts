import { useCallback, useReducer } from "react";
import type { ModelData } from "../types/model";

const MAX_HISTORY = 60;

type State = {
  model: ModelData;
  past: ModelData[];
  future: ModelData[];
  dirty: boolean;
};

type Action =
  | { type: "replace"; model: ModelData; clean?: boolean }
  | { type: "update"; updater: (prev: ModelData) => ModelData }
  | { type: "undo" }
  | { type: "redo" }
  | { type: "saved" };

function reducer(state: State, action: Action): State {
  switch (action.type) {
    case "replace":
      return {
        model: action.model,
        past: [],
        future: [],
        dirty: !action.clean,
      };
    case "update": {
      const next = action.updater(state.model);
      if (next === state.model) return state;
      return {
        model: next,
        past: [...state.past.slice(-MAX_HISTORY + 1), state.model],
        future: [],
        dirty: true,
      };
    }
    case "undo": {
      if (state.past.length === 0) return state;
      const previous = state.past[state.past.length - 1];
      return {
        model: previous,
        past: state.past.slice(0, -1),
        future: [state.model, ...state.future],
        dirty: true,
      };
    }
    case "redo": {
      if (state.future.length === 0) return state;
      const next = state.future[0];
      return {
        model: next,
        past: [...state.past, state.model],
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

export function useModelHistory(initial: ModelData) {
  const [state, dispatch] = useReducer(reducer, {
    model: initial,
    past: [],
    future: [],
    dirty: false,
  });

  const replace = useCallback((model: ModelData, clean = false) => {
    dispatch({ type: "replace", model, clean });
  }, []);

  const update = useCallback((updater: (prev: ModelData) => ModelData) => {
    dispatch({ type: "update", updater });
  }, []);

  const undo = useCallback(() => dispatch({ type: "undo" }), []);
  const redo = useCallback(() => dispatch({ type: "redo" }), []);
  const markSaved = useCallback(() => dispatch({ type: "saved" }), []);

  return {
    model: state.model,
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
