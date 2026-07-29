# Easy Text Animation (Free)

**Animate UMG text one character at a time — for Unreal Engine 5.7.**
Drop in an *Animated Text Block*, pick an effect, tweak a few numbers — done.
Full Blueprint control and a drag-and-drop showroom.

> Supports Thai and other complex scripts: text is split by **grapheme
> cluster** and rendered through HarfBuzz — vowels/tone marks attach
> correctly, and letter-spacing / skew / outline all work.

---

## 📥 Install

1. Copy the plugin folder into `<your project>/Plugins/`
2. Open the project → the editor will ask to compile — answer **Yes**
3. If it doesn't appear automatically, enable it under *Edit → Plugins → UI → Easy Text Animation*

On first editor launch, a **Text Test** window pops up showing every effect
(uncheck *Show this window when the editor starts* if you don't want it again).

---

## 🚀 Tutorial — 4 steps to get started

### 1. Place the widget
Open a **Widget Blueprint** → in the Palette (left side) search for
**Animated Text Block** (category *Text Animator*) → drag it onto the canvas.

### 2. Set text + pick an effect
Select the widget → look at **Details** (right side):
- **Text** — type your text
- **Effect** — choose an effect: Typewriter, Fade In, Slide Up, Wave
- **Timing Mode** — Per Character (fixed stagger) or Fixed Duration (whole text fits a total time)
- **Params** — adjust the numbers (Details only shows the params that effect uses)

| Param | What it controls |
|---|---|
| Char Anim Duration | Time each character takes to finish its own animation (Fade/Slide) |
| Stagger | Delay between consecutive characters, when Timing Mode = Per Character |
| Total Duration | Whole text's finish time, when Timing Mode = Fixed Duration |
| Amplitude | Pixel distance for Slide Up / Wave |
| Frequency | Hz for Wave |
| Loop | Wave loops until `Deactivate()` |

> The effect plays automatically when the widget appears (bAutoActivate is on).
> Want to preview live in the Designer? Enable **bPreviewInDesigner**.

### 3. Trigger it from Blueprint (optional)
Drag off the widget and call:
- **Activate** — replay from the start
- **Deactivate** — stop and show the full text
- **Set Text** (bReplay = true) — change the text and replay
- **Set Effect**, **Set Params**, **Set Preset** — change animation on the fly
- **Skip To End** — jump straight to the finished state
- **Is Active** — check whether it's currently animating
- Event **On Animation Finished** — fires when playback ends (hook up dialogue here)

---

## 🎨 Preset Showroom (shortcut)

Open **Window → Text Animator Presets**:

1. **Adjust effect + text + font/size** at the top → see a live preview
2. Cards below are the built-in presets
   - **Drag a card into the Designer** → get a fully configured Animated Text Block
   - **Click a card** → load its values into the editor above

---

## 🎬 Available effects

| Effect | Description |
|---|---|
| Typewriter | Classic type-on reveal |
| Fade In | Smooth per-character fade |
| Slide Up | Rise into place |
| Wave | Flowing sine motion (loops) |

Every effect supports easing (Linear through BounceOut).

---

## ⬆️ Want more?

**Easy Text Animation Pro** adds 14 more effects (Decoder, Rainbow, Glitch,
Bounce, Shake, Scale Pop, Blur, 4-way Slide, and more), Rich Text tags
(mix effects/colors inline), exit animations, custom easing curves, typing
sound, playback control (pause/resume/speed), and saving your own presets.

---

## ⚠️ Known limits

- One font/size per block
- Text may look slightly soft at DPI scales other than 100%
- Wrapped lines still count trailing whitespace in their measured width
  (minor offset on center/right justification)
- No Rich Text tags, custom easing curves, typing sound, playback control,
  or preset saving in this (Free) edition — see Pro
