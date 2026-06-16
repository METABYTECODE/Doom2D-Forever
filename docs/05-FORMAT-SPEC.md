# 05 — Спецификация форматов

> **Legacy formats** — для import tools (`tools/`). Runtime игры читает **modern format** из `assets/content/`.  
> См. [06-REVISED-STRATEGY.md](06-REVISED-STRATEGY.md).

---

## 0. Modern format (runtime target)

### 0.1 manifest.json

```json
{
  "version": 1,
  "assets": [
    {
      "id": "music.prostota",
      "type": "music",
      "path": "music/prostota.ogg",
      "legacy_refs": ["standart.wad:D2DMUS/ПРОСТОТА"]
    }
  ]
}
```

### 0.2 Asset ID rules

- ASCII slug: `category.name` (`tex.hud`, `sfx.door_open`, `music.prostota`)
- Files on disk: ASCII snake_case
- Cyrillic only in `meta` fields, never in paths

### 0.3 Map format v1 (JSON)

Converted from legacy by `d2df-map-import`. Resource refs use asset IDs, not WAD paths.

---

## 1. DFWAD Archive Format (legacy import)

Основной формат игровых ресурсов. **Не путать с id Software WAD.**

### 1.1 Header (8 bytes)

```
Offset  Size  Field        Value
0       5     magic        "DFWAD" (0x44 0x46 0x57 0x41 0x44)
5       1     version      0x01
6       2     file_count   uint16 LE
```

### 1.2 Directory Entry (24 bytes each)

```
Offset  Size  Field
0       16    name         NUL-terminated, '/' stored as '_'
16      4     data_offset  uint32 LE (0 for directory markers)
20      4     packed_size  uint32 LE (0 for directory markers)
```

**Directory marker:** `data_offset == 0 && packed_size == 0` → sets current path prefix.

**File entry:** data at `data_offset`, compressed size = `packed_size`.

### 1.3 Compression

- Algorithm: **raw zlib deflate** (no zlib header)
- C++ init: `inflateInit2(&strm, -MAX_WBITS)`
- Unpacked size: unknown until full decompression

### 1.4 Name encoding

- Max 16 bytes including NUL
- `\` → `/` on read
- `/` → `_` in stored name (flat namespace with path prefixes via directory markers)

### 1.5 Example structure (game.WAD)

```
DFWAD\x01 + count=588
[MUSIC\0\0\0...] [offset=0] [size=0]         ← directory marker
[INTERMUS\0\0...] [offset=1234] [size=5678]  ← file
[TEXTURES\0...] [offset=0] [size=0]          ← directory marker
[HUD\0\0\0...] [offset=9012] [size=3456]     ← file
...
```

### 1.6 Nested DFWAD — animated textures

Some texture entries (e.g. `standart.WAD:D2DTEXTURES/COL5A0`) are **not raw images** but a **nested DFWAD** blob. Pascal detects via `isWadData()` in `utils.pas` and loads in `CreateAnimTexture()` (`g_map.pas`).

```
COL5A0 (outer WAD entry, zlib-compressed)
  └── DFWAD\x01 (nested, 4 entries)
       ├── TEXT/          ← directory
       ├── TEXT/ANIM      ← INI config (plain text)
       ├── TEXTURES/      ← directory
       └── TEXTURES/COL5A ← TGA sprite sheet (zlib, may use zlib header 0x78 0xDA)
```

**TEXT/ANIM** example:
```ini
resource=COL5A
framecount=2
framewidth=64
frameheight=64
waitcount=10
backanimation=0
```

- `COL5A0` / `COL5B0` — separate animation phases (A/B), each is its own nested WAD
- Runtime uses `g_Frames_CreateMemory()` to slice sprite sheet into frames
- **Import tool:** `unwrap_nested_resource()` in `nested_resource.cpp` extracts `TEXTURES/{resource}` → TGA → PNG
- Animation metadata is **not** exported to runtime (sprite sheet PNG is enough for now)

Reference: `src/game/g_map.pas` lines ~1164–1236, `src/game/g_textures.pas` (`ANIM` savestate signature).

### 1.7 Verified archives

| File | Magic | Entries | Size |
|------|-------|---------|------|
| game.WAD | DFWAD\x01 | 588 | ~9 MB |
| standart.WAD | DFWAD\x01 | 619 | ~5 MB |
| editor.WAD | DFWAD\x01 | 6 | ~7 KB |
| shrshade.WAD | DFWAD\x01 | 141 | ~21 KB |

---

## 2. ZIP / DFZ Format

### 2.1 Extension aliases

```cpp
const char* WAD_EXTENSIONS[] = {
    ".dfz", ".wad", ".dfwad", ".pk3", ".pak", ".zip", ".dfdf"
};
```

### 2.2 Magic detection

| Magic bytes | Format |
|-------------|--------|
| `DFWAD\x01` | DFWAD (see section 1) |
| `PK\x03\x04` | ZIP local header |
| `PK\x05\x06` | ZIP end of central directory |
| `PACK` | Quake PAK |
| `SPAK` | SiN SPAK |

**Critical:** `.dfz` extension does NOT imply DFWAD. Most map/model `.dfz` files are ZIP.

### 2.3 SFS volume prefixes (optional)

```
dfz:path/file.dfz
dfwad:path/file.wad
zip:path/file.zip
pk3:path/file.pk3
pak:path/file.pak
```

---

## 3. Resource Path Syntax

### 3.1 Format

```
<archive_path>:\<internal_path>\<resource_name>
```

### 3.2 Examples

```
game.WAD:\TEXTURES\HUD
Standart.wad:D2DMUS\ПРОСТОТА
megawads/DOOM2D.WAD:\MAP01
path/to/map.wad:\MAPS/MyLevel
```

### 3.3 Parsing rules

1. Find `:` after recognized WAD extension
2. Everything before `:` = archive path (normalize `\` → `/`)
3. Everything after `:` = internal path (normalize `\` → `/`)
4. Lookup: strip extension from resource name, case-insensitive match
5. Last match wins (scan directory backwards)

### 3.4 Search directories

| Variable | Default | Contents |
|----------|---------|----------|
| DataDirs | `<exe>/data` | game.WAD, standart.WAD, editor.WAD |
| MapDirs | `<exe>/maps` | Map WADs/DFZs |
| MegawadDirs | `<exe>/maps/megawads` | Campaign packs |
| ModelDirs | `<exe>/data/models` | Player models |
| WadDirs | `<exe>/wads` | Supplemental WADs |

---

## 4. Map Format

Schema defined in `src/mapdef/mapdef.txt`, compiled by mapgen.

### 4.1 Binary Map

**Signature:** `MAP\x01` (4 bytes: 0x4D 0x41 0x50 0x01)

**Block stream:**

```
repeat:
    uint8   block_type     // 0 = end of map
    uint32  reserved       // always 0
    uint32  size           // payload size (LE)
    uint8[size] data       // block payload
until block_type == 0

// End marker: 00 00 00 00 00 00 00 00
```

**Block types:**

| ID | Record | Size | Content |
|----|--------|------|---------|
| 7 | map (header) | 452 | name, author, desc, music, sky, size |
| 1 | texture | 65 | path[64] + animated flag |
| 2 | panel | 18 | position, size, tex, type, alpha, flags |
| 3 | item | 10 | position, type, options |
| 4 | area | 10 | position, type, direction |
| 5 | monster | 10 | position, type, direction |
| 6 | trigger | 148 | position, size, enabled, tex, type, data[128] |
| 0 | — | — | End marker |

**Map header (452 bytes):**

| Offset | Size | Field |
|--------|------|-------|
| 0 | 32 | name (char[]) |
| 32 | 32 | author |
| 64 | 256 | description |
| 320 | 64 | music (resource path) |
| 384 | 64 | sky (resource path) |
| 448 | 4 | size (width: uint16 + height: uint16) |

### 4.2 Text Map

```
map {
    name "My Map"
    author "Mapper"
    size (1600 1200)
    music "Standart.wad:D2DMUS\ПРОСТОТА"
    sky "Standart.wad:D2DSKY\RSKY1"

    texture {
        path "mytexture.tga"
        animated false
    }

    panel {
        position (100 200)
        size (32 32)
        texture 0
        type PANEL_WALL
    }

    item { ... }
    monster { ... }
    trigger { ... }
    area { ... }
}
```

Detection: first token is `map` (when magic ≠ `MAP\x01`).

**Text-only fields** (lost in binary round-trip):
- Moving platform options (`move_speed`, `move_start`, etc.)
- Exoma script strings (`exoma_init`, `exoma_action`, etc.)
- `light_ambient`

### 4.3 Map enums (from mapdef.txt)

**Panel types (bitset):**
```
PANEL_NONE=0, PANEL_WALL=1, PANEL_BACK=2, PANEL_FORE=4,
PANEL_WATER=8, PANEL_ACID1=16, PANEL_ACID2=32, PANEL_STEP=64,
PANEL_LIFTUP=128, PANEL_LIFTDOWN=256, PANEL_OPENDOOR=512,
PANEL_CLOSEDOOR=1024, PANEL_BLOCKMON=2048, PANEL_LIFTLEFT=4096,
PANEL_LIFTRIGHT=8192
```

**Trigger types:** 30 types (TRIGGER_EXIT=1 ... TRIGGER_SCRIPT=29)

**Item types:** 40+ types (medkits, armor, weapons, ammo, keys, powerups)

**Monster types:** 20+ types (zombie, sergeant, imp, demon, etc.)

---

## 5. Save Game Format (DFSV v7)

### 5.1 Header

```
Magic: "DFSV"
Version: 7 (uint32 LE)
```

### 5.2 Contents

- Player states (position, health, inventory, weapons)
- Map state (panels, triggers, items, monsters)
- Game settings (mode, score, time)
- Projectile states
- Dynamic light states

Port strategy: read Pascal save format for compatibility, write new format with version bump.

---

## 6. Network Protocol (legacy reference — NOT ported)

Pascal protocol v188 documented here **for analysis only**. Phase 9 will use a **new protocol**.

### 6.0 Modern alternative (Phase 9)

- Versioned binary or protobuf/msgpack messages
- Content sync via manifest hash (SHA-256 of content/)
- ENet transport (same library, new message layout)

### 6.1 Legacy constants (reference)

| Constant | Value |
|----------|-------|
| NET_PROTOCOL_VER | 188 |
| GAME_VERSION | "0.667" |
| NET_MAXCLIENTS | 24 |
| NET_CHANNELS | 12 |
| NET_CHAN_RELIABLE | 1 |
| NET_CHAN_UNRELIABLE | 2 |
| NET_CHAN_DOWNLOAD | 11 |
| NET_PING_PORT | 0xDF2D (57133) |
| Default game port | 25666 |
| Default master port | 25665 |
| GAME_TICKS | 36 (UPS) |
| Download chunk size | 8192 |

### 6.2 Packet framing

Each ENet payload contains concatenated sub-messages:

```
[uint32_le message_size][message_body]  ×N
```

`message_body` = `[uint8 message_id][typed fields...]`

### 6.3 TMsg serialization

| Type | Format |
|------|--------|
| uint8/16/32, int32 | Little-endian |
| String | `uint8 length` (max 255) + raw bytes |
| MD5 digest | 16 raw bytes |
| Nested message | Raw byte copy |

### 6.4 Game message IDs

**Client → Server:**

| ID | Name | Payload |
|----|------|---------|
| 100 | NET_MSG_INFO | version, password, name, model, team... |
| 101 | NET_MSG_CHAT | text, mode |
| 108 | NET_MSG_REQFST | (empty) full state request |
| 112 | NET_MSG_PLRPOS | gTime, keys, dir, weapon act/select |
| 119 | NET_MSG_PLRSET | player settings |
| 120 | NET_MSG_CHEAT | cheat kind |
| 191 | NET_MSG_RCON_AUTH | password |
| 192 | NET_MSG_RCON_CMD | command |
| 195 | NET_MSG_VOTE_EVENT | start, command |

**Server → Client:**

| ID | Name | Typical channel |
|----|------|-----------------|
| 100 | NET_MSG_INFO | Reliable |
| 101–110 | Chat, GFX, sound, events, score | Mixed |
| 111–120 | Player create/pos/stats/del/dmg/die | Pos=unreliable |
| 121–123 | Items spawn/del/pos | Mixed |
| 131–135 | Monsters spawn/pos/state/shot/del | Mixed |
| 141–142 | Panel state/texture | Reliable |
| 151–152 | Trigger sound/music | Reliable |
| 161–163 | Projectiles add/pos/del | Mixed |
| 194 | NET_MSG_TIME_SYNC | Unreliable |
| 195 | Vote events | Reliable |

### 6.5 Download sub-protocol (channel 11)

| Server ID | Client ID | Purpose |
|-----------|-----------|---------|
| 10 DONE | 100 MAP_REQUEST | Transfer complete / request map |
| 11 FILE_INFO | 101 FILE_REQUEST | File metadata / request file |
| 12 CHUNK | 102 ABORT | Data chunk / abort |
| 13 ABORT | 103 START | Abort / resume from chunk |
| 14 MAP_INFO | 104 ACK | Map info / acknowledge |

Extended map info: `!` prefix + size + MD5 + name.

### 6.6 Master server protocol

| ID | Name | Direction |
|----|------|-----------|
| 200 | NET_MSG_ADD / NET_MMSG_UPD | Server → master: register |
| 201 | NET_MSG_RM / NET_MMSG_DEL | Server → master: unregister |
| 202 | NET_MSG_LIST / NET_MMSG_GET | Client → master: query list |

### 6.7 UDP Ping

```
Query:  "DF" + int64 client_timestamp
Response: "DF" + uint16 game_port + int64 timestamp + server_info + player_count + bot_count
```

---

## 7. Image Formats

### 7.1 In-WAD images

Loaded via stb_image (replacing Vamp Imaging). Supported: TGA, PNG, BMP, JPG, GIF.

Pipeline:
1. Read compressed bytes from WAD
2. Decode to RGBA8888
3. Upload to GPU texture
4. Optional: pad to power-of-two

### 7.2 Font format

**Texture font:** single atlas image + `.cfg` sidecar:
```ini
[FontMap]
CharWidth=8
CharHeight=8
Kerning=0
; per-char widths follow
```

**Char font:** per-glyph widths stored in atlas metadata.

### 7.3 FlexUI font (debug only)

```
Magic: "FUIFONT0"
```

Used by Holmes debugger, not core game.

---

## 8. Audio Formats

### 8.1 Loader chain (priority order)

1. Extension match (`.wav`, `.ogg`, `.mod`, `.mp3`, etc.)
2. Header sniff (RIFF, OggS, etc.)

### 8.2 Supported formats

| Format | Library | Usage |
|--------|---------|-------|
| WAV | dr_wav / SDL | SFX |
| Ogg Vorbis | stb_vorbis / libvorbis | Music, SFX |
| Opus | libopus | Music |
| MP3 | minimp3 / libmpg123 | Music |
| MOD/S3M/XM/IT | libmodplug / libxmp | Music |
| NSF/SPC/GYM | libgme | Retro music |
| MIDI | fluidsynth | Music |

In-WAD sounds often have no file extension in path (e.g., `SOUNDS\DOOROPEN`).

---

## 9. Player Model Format

Stored in `.wad` or `.dfz` (ZIP) in `data/models/`:

```
model.wad:\SPRITES\WALK1
model.wad:\WEAPONS\PISTOL
model.wad:\CONFIG\model.cfg
```

Model config defines animation sets, weapon attach points, gib sprites.

---

## 10. Localization Format

`.lng` files (e.g., `editor.ru_RU.lng`):

```ini
[Strings]
IDS_MAINMENU=Главное меню
IDS_OPTIONS=Настройки
...
```

Game uses compile-time enum indices mapped to string IDs.

---

## 11. Format Signatures Quick Reference

| Format | Hex signature | ASCII |
|--------|--------------|-------|
| DFWAD | `44 46 57 41 44 01` | DFWAD\x01 |
| ZIP local | `50 4B 03 04` | PK\x03\x04 |
| ZIP central | `50 4B 01 02` | PK\x01\x02 |
| ZIP end | `50 4B 05 06` | PK\x05\x06 |
| Quake PAK | `50 41 43 4B` | PACK |
| Binary map | `4D 41 50 01` | MAP\x01 |
| Save game | `44 46 53 56` | DFSV |
| WAV | `52 49 46 46 ?? ?? ?? ?? 57 41 56 45` | RIFF....WAVE |
| Ogg | `4F 67 67 53` | OggS |
| FlexUI font | `46 55 49 46 4F 4E 54 30` | FUIFONT0 |
| UDP ping | `44 46` | DF |

---

## 12. C++ Implementation Priority

| Priority | Component | Depends on |
|----------|-----------|------------|
| P0 | DFWAD reader | zlib |
| P0 | ZIP reader | miniz/libzip |
| P0 | Resource path parser | — |
| P0 | Binary map parser | mapgen output |
| P0 | NetMessage codec | — |
| P1 | Text map parser | mapgen output |
| P1 | stb_image texture loader | DFWAD reader |
| P1 | WAV/OGG sound loader | DFWAD reader |
| P2 | Save game reader | all entity types |
| P2 | FlexUI font (debug) | ZIP reader |
| P3 | PNG map generator | mapgen output |

---

## 13. Golden Test Vectors

Create these test files for byte-level validation:

```
tests/golden/
├── dfwad/
│   ├── game_wad_header.bin       # First 8 bytes of game.WAD
│   ├── game_wad_dir_entry_0.bin  # First directory entry
│   └── decompress_hud.bin        # Decompressed TEXTURES/HUD
├── map/
│   ├── map01_header.bin          # MAP01 header block
│   ├── map01_panels.bin          # Panel block from MAP01
│   └── text_map_simple.txt       # Minimal text map
├── net/
│   ├── msg_player_pos_client.bin # Client NET_MSG_PLRPOS
│   ├── msg_player_pos_server.bin # Server NET_MSG_PLRPOS
│   └── msg_info_response.bin     # Server NET_MSG_INFO
└── save/
    └── savgame1_header.bin       # DFSV header
```

Generate vectors by running Pascal tools with hex dump, or extracting from known-good files in `assets/`.
