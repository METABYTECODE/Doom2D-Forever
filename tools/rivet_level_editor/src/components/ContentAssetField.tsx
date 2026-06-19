import { useEffect, useMemo, useState } from "react";
import type { PackAsset } from "../lib/resource-pack";

interface Props {
  label: string;
  value: string;
  assets: PackAsset[];
  variant: "image" | "audio";
  emptyHint: string;
  onChange: (path: string) => void;
}

export function ContentAssetField({
  label,
  value,
  assets,
  variant,
  emptyHint,
  onChange,
}: Props) {
  const [pickerOpen, setPickerOpen] = useState(false);
  const selected = useMemo(
    () => assets.find((asset) => asset.id === value) ?? null,
    [assets, value],
  );

  useEffect(() => {
    if (!pickerOpen) return;
    const onKey = (e: KeyboardEvent) => {
      if (e.key === "Escape") setPickerOpen(false);
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [pickerOpen]);

  const categories = useMemo(() => {
    const set = new Set<string>();
    for (const asset of assets) {
      set.add(asset.category);
    }
    return [...set].sort();
  }, [assets]);

  return (
    <>
      <label>
        {label}
        <div className="asset-picker-row">
          <select value={value} onChange={(e) => onChange(e.target.value)}>
            <option value="">(none)</option>
            {assets.map((asset) => (
              <option key={asset.id} value={asset.id}>
                {asset.label}
              </option>
            ))}
          </select>
          <button type="button" onClick={() => setPickerOpen(true)} disabled={assets.length === 0}>
            Browse…
          </button>
          {value && (
            <button type="button" className="asset-clear-btn" onClick={() => onChange("")} title="Clear">
              ✕
            </button>
          )}
        </div>
      </label>

      {selected && variant === "image" && (
        <div className="asset-preview-wrap">
          <img src={selected.url} alt={selected.label} className="asset-preview-image" />
          <span className="mono muted">{selected.id}</span>
        </div>
      )}

      {selected && variant === "audio" && (
        <div className="asset-preview-wrap">
          <audio controls src={selected.url} className="asset-preview-audio" />
          <span className="mono muted">{selected.id}</span>
        </div>
      )}

      {!selected && value && <p className="hint warn">Missing asset id: {value}</p>}

      {assets.length === 0 && <p className="hint">{emptyHint}</p>}

      {pickerOpen && (
        <div className="modal-backdrop" onPointerDown={() => setPickerOpen(false)}>
          <div
            className="content-asset-modal"
            onPointerDown={(e) => e.stopPropagation()}
            role="dialog"
            aria-modal="true"
            aria-label={`Pick ${label.toLowerCase()}`}
          >
            <header className="tileset-modal-header">
              <strong>Pick {label.toLowerCase()}</strong>
              <button type="button" className="modal-close" onClick={() => setPickerOpen(false)}>
                ✕
              </button>
            </header>
            <div className="content-asset-modal-body">
              {assets.length === 0 ? (
                <p className="muted">{emptyHint}</p>
              ) : variant === "image" ? (
                categories.map((category) => (
                  <section key={category} className="content-asset-group">
                    <h4>{category}</h4>
                    <div className="content-asset-grid">
                      {assets
                        .filter((asset) => asset.category === category)
                        .map((asset) => (
                          <button
                            key={asset.id}
                            type="button"
                            className={`content-asset-card${value === asset.id ? " selected" : ""}`}
                            onClick={() => {
                              onChange(asset.id);
                              setPickerOpen(false);
                            }}
                          >
                            <img src={asset.url} alt={asset.label} />
                            <span>{asset.label}</span>
                          </button>
                        ))}
                    </div>
                  </section>
                ))
              ) : (
                categories.map((category) => (
                  <section key={category} className="content-asset-group">
                    <h4>{category}</h4>
                    <ul className="content-asset-list">
                      {assets
                        .filter((asset) => asset.category === category)
                        .map((asset) => (
                          <li key={asset.id}>
                            <button
                              type="button"
                              className={`content-asset-list-item${value === asset.id ? " selected" : ""}`}
                              onClick={() => {
                                onChange(asset.id);
                                setPickerOpen(false);
                              }}
                            >
                              <span className="mono">{asset.label}</span>
                              <span className="muted">{asset.id}</span>
                            </button>
                          </li>
                        ))}
                    </ul>
                  </section>
                ))
              )}
            </div>
          </div>
        </div>
      )}
    </>
  );
}
