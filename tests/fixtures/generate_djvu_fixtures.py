#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import os
import shutil
import subprocess
from pathlib import Path
from xml.etree import ElementTree as ET

from PIL import Image, ImageDraw, ImageFont


REPO_ROOT = Path(__file__).resolve().parents[2]
DEE5_BIN = Path(os.environ.get("DEE5_BIN", Path.home() / "Soft" / "DEE5" / "bin"))
DJVULIBRE_BIN = Path(os.environ.get("DJVULIBRE_BIN", REPO_ROOT / "windows" / "build" / "Release" / "x64"))
OUT_DIR = Path("tests/fixtures/reference")
TMP_DIR = OUT_DIR / "_tmp_build"

W = 800
H = 600


def run(args: list[str]) -> None:
    subprocess.run(args, check=True)


def rgb(hex_color: str) -> tuple[int, int, int]:
    c = hex_color.lstrip("#")
    return int(c[0:2], 16), int(c[2:4], 16), int(c[4:6], 16)


def font(size: int) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    for name in ("arial.ttf", "DejaVuSans.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            pass
    return ImageFont.load_default()


def draw_centered_text(
    draw: ImageDraw.ImageDraw,
    text: str,
    box: tuple[int, int, int, int],
    fill: tuple[int, int, int],
    size: int,
) -> None:
    x1, y1, x2, y2 = box
    fnt = font(size)
    bbox = draw.textbbox((0, 0), text, font=fnt)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    x = x1 + max(0, (x2 - x1 - tw) // 2)
    y = y1 + max(0, (y2 - y1 - th) // 2)
    draw.text((x, y), text, fill=fill, font=fnt)


def save_rgb_bmp(path: Path, bg_hex: str, text: str | None = None, text_size: int = 42) -> None:
    img = Image.new("RGB", (W, H), rgb(bg_hex))
    if text:
        draw = ImageDraw.Draw(img)
        draw_centered_text(draw, text, (0, 0, W, H), (25, 40, 80), text_size)
    img.save(path, format="BMP")


def save_indexed_bmp(path: Path) -> None:
    img = Image.new("P", (W, H))
    palette = []
    colors = [
        "#E6F0FF", "#D9E8FF", "#CCE0FF", "#BFD8FF",
        "#B0CEFF", "#A0C2FF", "#90B6F8", "#7FA9F0",
        "#F3F7FF", "#EAF2FF", "#D6E5FF", "#C7DAFF",
        "#B8CFFF", "#A8C2F5", "#99B7EA", "#89A9DF",
    ]
    for color in colors:
        palette.extend(rgb(color))
    palette.extend([0, 0, 0] * (256 - len(colors)))
    img.putpalette(palette)

    for y in range(H):
        for x in range(W):
            idx = ((x // 50) + (y // 50)) % len(colors)
            img.putpixel((x, y), idx)

    img.save(path, format="BMP")


def save_mask_heavy_bmp(path: Path) -> None:
    img = Image.new("RGB", (W, H), rgb("#FFF7E8"))
    draw = ImageDraw.Draw(img)
    fnt = font(16)

    for y in range(30, H - 30, 22):
        for x in range(24, W - 140, 72):
            draw.rectangle((x, y, x + 8, y + 8), fill=(0, 0, 0))
            draw.ellipse((x + 14, y, x + 24, y + 10), fill=(0, 0, 0))
            draw.text((x + 32, y - 4), "MASK42", fill=(0, 0, 0), font=fnt)

    draw_centered_text(draw, "BW_MASK_HEAVY", (0, 0, W, 80), (0, 0, 0), 26)
    img.save(path, format="BMP")


def save_shared_page_bmp(path: Path, page_marker: str, bg_hex: str) -> None:
    img = Image.new("RGB", (W, H), rgb(bg_hex))
    draw = ImageDraw.Draw(img)
    glyph = "SHARED GLYPHS 0123456789 ABCDEFGH"
    fnt = font(20)

    for row in range(7):
        draw.text((55, 70 + row * 55), glyph, fill=(10, 10, 10), font=fnt)

    draw.rounded_rectangle((80, 470, 720, 550), radius=18, outline=(30, 55, 90), width=3)
    draw_centered_text(draw, page_marker, (90, 480, 710, 545), (30, 55, 90), 30)
    img.save(path, format="BMP")


def image_to_djvu(src_img: Path, dst_djvu: Path, options: list[str] | None = None) -> None:
    args = [str(DEE5_BIN / "documenttodjvu.exe")]
    if options:
        args.extend(options)
    args.extend([str(src_img), str(dst_djvu)])
    run(args)


def multi_images_to_bundled(src_imgs: list[Path], dst_djvu: Path, options: list[str] | None = None) -> None:
    args = [str(DEE5_BIN / "documenttodjvu.exe")]
    if options:
        args.extend(options)
    args.extend([str(p) for p in src_imgs])
    args.append(str(dst_djvu))
    run(args)


def make_hidden_text(parent: ET.Element, hidden_text: str, page_width: int = W, page_height: int = H) -> None:
    ht = ET.SubElement(parent, "HIDDENTEXT", {"coords": f"0,0,{page_width},{page_height}"})
    pc = ET.SubElement(ht, "PAGECOLUMN", {"coords": f"0,0,{page_width},{page_height}"})
    rg = ET.SubElement(pc, "REGION", {"coords": f"70,240,{page_width - 70},340"})
    pa = ET.SubElement(rg, "PARAGRAPH", {"coords": f"70,240,{page_width - 70},340"})
    ln = ET.SubElement(pa, "LINE", {"coords": f"70,240,{page_width - 70},340"})
    wd = ET.SubElement(ln, "WORD", {"coords": f"70,240,{page_width - 70},340"})
    wd.text = hidden_text


def apply_xml_updates(
    djvu_file: Path,
    hidden_text: str | None = None,
    links: list[dict[str, str]] | None = None,
) -> None:
    xml_file = TMP_DIR / f"{djvu_file.stem}.xml"
    run([str(DEE5_BIN / "djvutoxml.exe"), str(djvu_file), str(xml_file)])
    tree = ET.parse(xml_file)
    root = tree.getroot()
    body = root.find("./BODY")
    if body is None:
        raise RuntimeError(f"Missing BODY in {xml_file}")
    obj = body.find("OBJECT")
    if obj is None:
        raise RuntimeError(f"Missing OBJECT in {xml_file}")

    for child in list(obj):
        if child.tag == "HIDDENTEXT":
            obj.remove(child)
    if hidden_text:
        make_hidden_text(obj, hidden_text)

    map_el = body.find("MAP")
    if map_el is None:
        map_el = ET.SubElement(body, "MAP", {"name": djvu_file.name})
    for child in list(map_el):
        map_el.remove(child)
    if links:
        for item in links:
            ET.SubElement(
                map_el,
                "AREA",
                {
                    "shape": "rect",
                    "coords": item["coords"],
                    "href": item["href"],
                    "alt": item["alt"],
                    "bordertype": "solid",
                    "bordercolor": item.get("bordercolor", "#0000FF"),
                    "border": "1",
                },
            )

    tree.write(xml_file, encoding="utf-8", xml_declaration=True)
    run([str(DEE5_BIN / "djvuparsexml.exe"), str(xml_file)])


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def get_shared_resource_hint(djvu_file: Path) -> bool:
    dump = subprocess.run(
        [str(DJVULIBRE_BIN / "djvudump.exe"), str(djvu_file)],
        check=True,
        capture_output=True,
        text=True,
    ).stdout
    return "Djbz" in dump or "Sjbz" in dump


def write_manifest(lines: list[str]) -> None:
    (OUT_DIR / "manifest.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_sha256sums() -> None:
    hash_targets = []
    for path in sorted(OUT_DIR.rglob("*")):
        if not path.is_file():
            continue
        if path.name == "SHA256SUMS.txt":
            continue
        if "_tmp_build" in path.parts:
            continue
        hash_targets.append(path)

    lines = [f"{sha256_file(path)}  {path.relative_to(OUT_DIR).as_posix()}" for path in hash_targets]
    (OUT_DIR / "SHA256SUMS.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")


def ensure_tools() -> None:
    required = [
        "documenttodjvu.exe",
        "djvutoxml.exe",
        "djvuparsexml.exe",
        "djvubundle.exe",
        "djvujoin.exe",
    ]
    for tool in required:
        if not (DEE5_BIN / tool).exists():
            raise FileNotFoundError(f"Missing tool: {DEE5_BIN / tool}")
    if not (DJVULIBRE_BIN / "djvudump.exe").exists():
        raise FileNotFoundError(f"Missing tool: {DJVULIBRE_BIN / 'djvudump.exe'}")


def reset_output_dirs() -> None:
    if OUT_DIR.exists():
        shutil.rmtree(OUT_DIR)
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    TMP_DIR.mkdir(parents=True, exist_ok=True)


def generate_reference_fixtures() -> None:
    reset_output_dirs()

    save_rgb_bmp(TMP_DIR / "bg_light.bmp", "#E6F0FF")
    save_rgb_bmp(TMP_DIR / "marker.bmp", "#E6F0FF", "ROTATION_MARKER", 36)
    save_rgb_bmp(TMP_DIR / "ascii_marker.bmp", "#E6F0FF", "ASCII_MARKER", 40)
    save_rgb_bmp(TMP_DIR / "visible_text.bmp", "#E6F0FF", "VISIBLE_TEXT_PAGE2_456", 38)
    save_rgb_bmp(TMP_DIR / "visible_text_p3.bmp", "#FFF4E0", "VISIBLE_TEXT_PAGE3_789", 38)
    save_indexed_bmp(TMP_DIR / "palette_indexed.bmp")
    save_mask_heavy_bmp(TMP_DIR / "bw_mask_heavy.bmp")
    save_shared_page_bmp(TMP_DIR / "shared_page1.bmp", "PAGE_ONE", "#EAF4FF")
    save_shared_page_bmp(TMP_DIR / "shared_page2.bmp", "PAGE_TWO", "#EEF7EA")

    sp_anno_only_links = OUT_DIR / "sp_anno_only_links.djvu"
    image_to_djvu(TMP_DIR / "bg_light.bmp", sp_anno_only_links)
    apply_xml_updates(
        sp_anno_only_links,
        links=[
            {
                "coords": "30,30,290,90",
                "href": "https://example.com/ref?x=1#top",
                "alt": "external-ref",
                "bordercolor": "#0055AA",
            },
            {
                "coords": "30,110,290,170",
                "href": "#sp_anno_only_links.djvu",
                "alt": "self-link",
                "bordercolor": "#AA2200",
            },
        ],
    )

    sp_hidden_only_no_visible = OUT_DIR / "sp_hidden_only_no_visible.djvu"
    image_to_djvu(TMP_DIR / "bg_light.bmp", sp_hidden_only_no_visible)
    apply_xml_updates(sp_hidden_only_no_visible, hidden_text="HIDDEN_ONLY_TEXT_123")

    for angle in ("90", "180", "270"):
        image_to_djvu(
            TMP_DIR / "marker.bmp",
            OUT_DIR / f"sp_rotation_{angle}.djvu",
            options=[f"--rotate={angle}"],
        )

    sp_palette_indexed_bg = OUT_DIR / "sp_palette_indexed_bg.djvu"
    image_to_djvu(TMP_DIR / "palette_indexed.bmp", sp_palette_indexed_bg)

    sp_bw_mask_heavy = OUT_DIR / "sp_bw_mask_heavy.djvu"
    image_to_djvu(
        TMP_DIR / "bw_mask_heavy.bmp",
        sp_bw_mask_heavy,
        options=["--fg-subsample=1", "--bg-subsample=3", "--pages-per-dict=1"],
    )

    mp3_page1 = TMP_DIR / "mp3_page1.djvu"
    mp3_page2 = TMP_DIR / "mp3_page2.djvu"
    mp3_page3 = TMP_DIR / "mp3_page3.djvu"
    image_to_djvu(TMP_DIR / "bg_light.bmp", mp3_page1)
    image_to_djvu(TMP_DIR / "visible_text.bmp", mp3_page2)
    image_to_djvu(TMP_DIR / "visible_text_p3.bmp", mp3_page3)
    apply_xml_updates(
        mp3_page3,
        hidden_text="HIDDEN_TEXT_PAGE3_XYZ",
        links=[
            {"coords": "25,25,250,85", "href": "#1", "alt": "p3-to-p1", "bordercolor": "#008800"},
            {
                "coords": "25,105,250,165",
                "href": "https://example.com/p3?src=djvu",
                "alt": "external-p3",
                "bordercolor": "#0000AA",
            },
        ],
    )

    mp3_bundled = OUT_DIR / "mp3_bundled_mixed_layers.djvu"
    run([str(DEE5_BIN / "djvubundle.exe"), str(mp3_page1), str(mp3_page2), str(mp3_page3), str(mp3_bundled)])

    mp3_indirect_dir = OUT_DIR / "mp3_indirect_mixed_layers"
    mp3_indirect_dir.mkdir(parents=True, exist_ok=True)
    run(
        [
            str(DEE5_BIN / "djvujoin.exe"),
            str(mp3_page1),
            str(mp3_page2),
            str(mp3_page3),
            str(mp3_indirect_dir / "index.djvu"),
        ]
    )

    mp2_shared = OUT_DIR / "mp2_bundled_shared_resources.djvu"
    multi_images_to_bundled(
        [TMP_DIR / "shared_page1.bmp", TMP_DIR / "shared_page2.bmp"],
        mp2_shared,
        options=["--pages-per-dict=2", "--fg-subsample=1", "--bg-subsample=3"],
    )

    sp_utf8_hidden = OUT_DIR / "sp_utf8_multilang_hidden.djvu"
    image_to_djvu(TMP_DIR / "ascii_marker.bmp", sp_utf8_hidden)
    apply_xml_updates(sp_utf8_hidden, hidden_text="Привіт DjVu δοκιμή 日本語 test 123")

    sp_links_edgecases = OUT_DIR / "sp_links_edgecases.djvu"
    image_to_djvu(TMP_DIR / "ascii_marker.bmp", sp_links_edgecases)
    apply_xml_updates(
        sp_links_edgecases,
        links=[
            {
                "coords": "30,30,330,90",
                "href": "https://example.com/a%20b?q=1&x=%2F#frag",
                "alt": "encoded-http",
                "bordercolor": "#0044AA",
            },
            {
                "coords": "30,110,330,170",
                "href": "mailto:test@example.com?subject=DjVu%20Test",
                "alt": "mailto-link",
                "bordercolor": "#AA5500",
            },
            {
                "coords": "30,190,330,250",
                "href": "#sp_links_edgecases.djvu",
                "alt": "self-link",
                "bordercolor": "#008844",
            },
        ],
    )

    shared_hint = get_shared_resource_hint(mp2_shared)
    manifest_lines = [
        "sp_anno_only_links.djvu -> pages:1,bg:solid(#E6F0FF),visible:none,hidden:none,links:external+self,contains_text:false,contains_anno:true,format:bundled",
        "sp_hidden_only_no_visible.djvu -> pages:1,bg:solid,visible:none,hidden:HIDDEN_ONLY_TEXT_123,links:none,contains_text:true,contains_anno:false,format:bundled",
        "sp_rotation_90.djvu -> pages:1,bg:solid(#E6F0FF),visible:ROTATION_MARKER,hidden:none,links:none,rotation:90,format:bundled",
        "sp_rotation_180.djvu -> pages:1,bg:solid(#E6F0FF),visible:ROTATION_MARKER,hidden:none,links:none,rotation:180,format:bundled",
        "sp_rotation_270.djvu -> pages:1,bg:solid(#E6F0FF),visible:ROTATION_MARKER,hidden:none,links:none,rotation:270,format:bundled",
        "sp_palette_indexed_bg.djvu -> pages:1,bg:indexed-palette(16 colors),visible:none,hidden:none,links:none,format:bundled",
        "sp_bw_mask_heavy.djvu -> pages:1,bg:simple,visible:dense-mask-glyphs,hidden:none,links:none,format:bundled",
        "mp3_bundled_mixed_layers.djvu -> pages:3,p1:bg-only,p2:bg+visible,p3:bg+visible+hidden+links(internal->p1,external),format:bundled",
        "mp3_indirect_mixed_layers/index.djvu -> pages:3,p1:bg-only,p2:bg+visible,p3:bg+visible+hidden+links(internal->p1,external),format:indirect",
        f"mp2_bundled_shared_resources.djvu -> pages:2,visible:shared-glyph-grid,hidden:none,links:none,shared_resources:{str(shared_hint).lower()},format:bundled",
        "sp_utf8_multilang_hidden.djvu -> pages:1,visible:ASCII_MARKER,hidden:Привіт DjVu δοκιμή 日本語 test 123,links:none,contains_text:true,format:bundled",
        "sp_links_edgecases.djvu -> pages:1,visible:ASCII_MARKER,hidden:none,links:https+mailto+self,contains_anno:true,format:bundled",
    ]
    write_manifest(manifest_lines)
    write_sha256sums()
    shutil.rmtree(TMP_DIR, ignore_errors=True)


def main() -> None:
    ensure_tools()
    generate_reference_fixtures()
    print(f"Generated fixtures in: {OUT_DIR.resolve()}")


if __name__ == "__main__":
    main()
