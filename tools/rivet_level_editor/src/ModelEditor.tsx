import { useCallback, useEffect, useMemo, useRef, useState } from "react";

import { AtlasPickerModal } from "./components/AtlasPickerModal";
import { JsonPreview } from "./components/JsonPreview";
import {
  AnimationRail,
  ModelInspector,
  ModelMenuBar,
  ModelStatusBar,
  SpriteDock,
} from "./components/ModelEditorLayout";
import { ModelPreviewCanvas } from "./components/ModelPreviewCanvas";
import { useModelHistory } from "./hooks/useModelHistory";
import { suggestColumns } from "./lib/model-atlas";
import {
  applyFeetPivotPreset,
  createBlankModel,
  defaultAnimation,
  downloadModel,
  fullFrameHull,
  parseModelJson,
  serializeModel,
} from "./lib/model-io";
import {
  DEFAULT_PACK_ID,
  loadPackImage,
  loadResourcePack,
  type PackAsset,
} from "./lib/resource-pack";
import { BUILTIN_ANIMATIONS, type ModelAnimation } from "./types/model";

type PickerMode = "bind" | "add-frame";

export function ModelEditor() {
  const {
    model,
    dirty,
    replace,
    update,
    undo,
    redo,
    markSaved,
    canUndo,
    canRedo,
  } = useModelHistory(createBlankModel());

  const pack = useMemo(
    () => loadResourcePack(model.resource_pack || DEFAULT_PACK_ID),
    [model.resource_pack],
  );

  const atlases = pack.spriteAtlases;
  const [atlasImages, setAtlasImages] = useState<Map<string, HTMLImageElement>>(new Map());
  const [selectedAnim, setSelectedAnim] = useState<string>(BUILTIN_ANIMATIONS[0]);
  const [frameIndex, setFrameIndex] = useState(0);
  const [playing, setPlaying] = useState(false);
  const [showJson, setShowJson] = useState(false);
  const [pickerOpen, setPickerOpen] = useState(false);
  const [pickerMode, setPickerMode] = useState<PickerMode>("bind");
  const [pickerAtlasId, setPickerAtlasId] = useState("");

  const fileInputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    let cancelled = false;
    const loadAll = async () => {
      const next = new Map<string, HTMLImageElement>();
      for (const atlas of atlases) {
        const image = await loadPackImage(atlas);
        if (image && !cancelled) next.set(atlas.id, image);
      }
      if (!cancelled) setAtlasImages(next);
    };
    void loadAll();
    return () => {
      cancelled = true;
    };
  }, [atlases]);

  useEffect(() => {
    setFrameIndex(0);
    setPlaying(false);
  }, [selectedAnim, model.animations[selectedAnim]?.atlas]);

  const currentAnim = model.animations[selectedAnim] ?? null;
  const activeAtlasId = currentAnim?.atlas || atlases[0]?.id || "";
  const previewFrameId =
    currentAnim?.frames[Math.min(frameIndex, Math.max(0, (currentAnim?.frames.length ?? 1) - 1))]
      ?.id ?? 0;
  const currentAtlas = atlases.find((a) => a.id === activeAtlasId);
  const currentImage = activeAtlasId ? (atlasImages.get(activeAtlasId) ?? null) : null;

  const pickerAnim = useMemo((): ModelAnimation | null => {
    if (currentAnim) return currentAnim;
    if (!pickerAtlasId) return null;
    return defaultAnimation(pickerAtlasId);
  }, [currentAnim, pickerAtlasId]);

  const status = useMemo(() => {
    const animCount = Object.keys(model.animations).length;
    return `${model.id} · ${animCount} anim(s) · ${atlases.length} atlas PNG(s)${dirty ? " · unsaved" : ""}`;
  }, [atlases.length, dirty, model.animations, model.id]);

  const loadFile = useCallback(
    async (file: File) => {
      const text = await file.text();
      const parsed = parseModelJson(text);
      replace(parsed, true);
      setSelectedAnim(BUILTIN_ANIMATIONS[0]);
      markSaved();
    },
    [markSaved, replace],
  );

  const ensureAnimation = useCallback(() => {
    update((prev) => {
      if (prev.animations[selectedAnim]) return prev;
      const firstAtlas = atlases[0]?.id ?? "";
      return {
        ...prev,
        animations: {
          ...prev.animations,
          [selectedAnim]: defaultAnimation(firstAtlas),
        },
      };
    });
  }, [atlases, selectedAnim, update]);

  const patchAnimation = useCallback(
    (patch: Partial<ModelAnimation>) => {
      update((prev) => {
        const existing = prev.animations[selectedAnim] ?? defaultAnimation();
        return {
          ...prev,
          animations: {
            ...prev.animations,
            [selectedAnim]: { ...existing, ...patch },
          },
        };
      });
    },
    [selectedAnim, update],
  );

  const onAtlasChange = useCallback(
    (atlasId: string) => {
      if (!atlasId) return;
      const image = atlasImages.get(atlasId);
      const cols = image
        ? suggestColumns(image.width, currentAnim?.frame_width ?? 64)
        : currentAnim?.columns ?? 1;

      update((prev) => {
        const existing = prev.animations[selectedAnim] ?? defaultAnimation(atlasId);
        const sameAtlas = existing.atlas === atlasId;
        return {
          ...prev,
          animations: {
            ...prev.animations,
            [selectedAnim]: {
              ...existing,
              atlas: atlasId,
              columns: sameAtlas ? existing.columns : cols,
              frames: sameAtlas ? existing.frames : [{ id: 0 }],
            },
          },
        };
      });
      setFrameIndex(0);
    },
    [atlasImages, currentAnim?.columns, currentAnim?.frame_width, selectedAnim, update],
  );

  const applyPickerFrame = useCallback(
    (atlasId: string, frameId: number, mode: PickerMode) => {
      update((prev) => {
        const existing = prev.animations[selectedAnim] ?? defaultAnimation(atlasId);
        if (mode === "bind") {
          return {
            ...prev,
            animations: {
              ...prev.animations,
              [selectedAnim]: { ...existing, atlas: atlasId, frames: [{ id: frameId }] },
            },
          };
        }
        const frames = [...existing.frames, { id: frameId }];
        return {
          ...prev,
          animations: {
            ...prev.animations,
            [selectedAnim]: { ...existing, atlas: atlasId, frames },
          },
        };
      });
      if (mode === "bind") setFrameIndex(0);
      setPickerOpen(false);
    },
    [selectedAnim, update],
  );

  const openPicker = useCallback(
    (mode: PickerMode) => {
      const atlasId = activeAtlasId || atlases[0]?.id || "";
      if (!atlasId) {
        window.alert("No atlas PNGs in resourcepacks/dev/textures/sprites/");
        return;
      }
      if (!model.animations[selectedAnim]) {
        ensureAnimation();
      }
      setPickerAtlasId(atlasId);
      setPickerMode(mode);
      setPickerOpen(true);
    },
    [activeAtlasId, atlases, ensureAnimation, model.animations, selectedAnim],
  );

  const addCustomAnimation = useCallback(() => {
    const name = window.prompt("Animation id (e.g. CUSTOM_ATTACK):");
    if (!name) return;
    const id = name.trim().toUpperCase().replace(/\s+/g, "_");
    if (!id) return;
    if (model.animations[id]) {
      window.alert(`Animation "${id}" already exists.`);
      return;
    }
    update((prev) => ({
      ...prev,
      animations: {
        ...prev.animations,
        [id]: defaultAnimation(atlases[0]?.id ?? ""),
      },
    }));
    setSelectedAnim(id);
  }, [atlases, model.animations, update]);

  const removeAnimation = useCallback(
    (id: string) => {
      if (!BUILTIN_ANIMATIONS.includes(id as (typeof BUILTIN_ANIMATIONS)[number])) {
        if (!window.confirm(`Remove animation "${id}"?`)) return;
      }
      update((prev) => {
        const next = { ...prev.animations };
        delete next[id];
        return { ...prev, animations: next };
      });
      if (selectedAnim === id) {
        setSelectedAnim(BUILTIN_ANIMATIONS[0]);
      }
    },
    [selectedAnim, update],
  );

  return (
    <div className="editor-root model-editor-root">
      <ModelMenuBar
        dirty={dirty}
        canUndo={canUndo}
        canRedo={canRedo}
        showJson={showJson}
        onUndo={undo}
        onRedo={redo}
        onOpen={() => fileInputRef.current?.click()}
        onNew={() => {
          if (dirty && !window.confirm("Discard unsaved changes?")) return;
          replace(createBlankModel());
          setSelectedAnim(BUILTIN_ANIMATIONS[0]);
        }}
        onExport={() => {
          downloadModel(model);
          markSaved();
        }}
        onToggleJson={() => setShowJson((v) => !v)}
      />

      <div className={`editor-main model-editor-main ${showJson ? "with-json" : ""}`}>
        <AnimationRail
          model={model}
          builtinAnimations={BUILTIN_ANIMATIONS}
          selectedAnim={selectedAnim}
          onSelectAnim={setSelectedAnim}
          onAddAnim={addCustomAnimation}
          onRemoveAnim={removeAnimation}
        />

        <div className="editor-center model-editor-center">
          <div className="model-preview-area">
            <ModelPreviewCanvas
              image={currentImage}
              animation={currentAnim}
              pivot={model.pivot}
              collider={model.collider}
              playing={playing}
              frameIndex={frameIndex}
              onFrameIndexChange={setFrameIndex}
            />
          </div>
          <SpriteDock
            atlases={atlases}
            atlasImages={atlasImages}
            animation={currentAnim}
            activeAtlasId={activeAtlasId}
            previewFrameId={previewFrameId}
            animLabel={selectedAnim}
            onAtlasChange={onAtlasChange}
            onOpenAtlas={() => openPicker("bind")}
          />
        </div>

        <ModelInspector
          model={model}
          animation={currentAnim}
          animId={selectedAnim}
          atlases={atlases}
          atlasImages={atlasImages}
          playing={playing}
          frameIndex={frameIndex}
          onModelPatch={(patch) => update((prev) => ({ ...prev, ...patch }))}
          onPivotPatch={(patch) =>
            update((prev) => ({ ...prev, pivot: { ...prev.pivot, ...patch } }))
          }
          onColliderPatch={(patch) =>
            update((prev) => ({ ...prev, collider: { ...prev.collider, ...patch } }))
          }
          onApplyFeetPreset={() =>
            update((prev) => ({ ...prev, ...applyFeetPivotPreset(prev, currentAnim) }))
          }
          onApplyFullFrameHull={() => {
            if (!currentAnim) return;
            update((prev) => ({
              ...prev,
              collider: fullFrameHull(currentAnim.frame_width, currentAnim.frame_height),
            }));
          }}
          onAnimPatch={patchAnimation}
          onAtlasChange={onAtlasChange}
          onAddFrame={() => openPicker("add-frame")}
          onRemoveFrame={(index) => {
            patchAnimation({
              frames: (currentAnim?.frames ?? []).filter((_, i) => i !== index),
            });
            setFrameIndex((i) => Math.max(0, Math.min(i, (currentAnim?.frames.length ?? 1) - 2)));
          }}
          onFrameIdChange={(index, id) => {
            if (!currentAnim) return;
            patchAnimation({
              frames: currentAnim.frames.map((frame, i) => (i === index ? { id } : frame)),
            });
          }}
          onTogglePlay={() => setPlaying((p) => !p)}
          onFrameIndexChange={setFrameIndex}
          onEnsureAnimation={ensureAnimation}
        />

        {showJson && <JsonPreview text={serializeModel(model)} />}
      </div>

      <ModelStatusBar text={status} />

      <AtlasPickerModal
        open={pickerOpen}
        title={pickerMode === "bind" ? "Pick atlas frame" : "Add animation frame"}
        atlases={atlases}
        atlasImages={atlasImages}
        activeAtlasId={pickerAtlasId || activeAtlasId}
        grid={
          pickerAnim ?? {
            frame_width: 64,
            frame_height: 64,
            columns: 1,
          }
        }
        selectedFrameId={pickerMode === "add-frame" ? previewFrameId : previewFrameId}
        onAtlasChange={(id) => {
          setPickerAtlasId(id);
          onAtlasChange(id);
        }}
        onGridChange={(patch) => patchAnimation(patch)}
        onSelect={(atlasId, frameId) => applyPickerFrame(atlasId, frameId, pickerMode)}
        onClose={() => setPickerOpen(false)}
      />

      <input
        ref={fileInputRef}
        type="file"
        accept=".json,.model,application/json"
        hidden
        onChange={(e) => {
          const file = e.target.files?.[0];
          if (file) void loadFile(file);
          e.target.value = "";
        }}
      />
    </div>
  );
}
