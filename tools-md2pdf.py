import markdown, pathlib, re

src = pathlib.Path('/home/user/nityamittal/system-design-notes.md').read_text()

# Drop the H1 + intro blurb; we render our own cover
body_md = src.split('---', 1)[1].lstrip() if src.startswith('# ') else src

def normalize(md):
    lines = md.split('\n')
    out = []
    fence = False
    LIST = re.compile(r'^\s*(?:[-*+] |\d+\. )')
    for ln in lines:
        if ln.lstrip().startswith('```'):
            fence = not fence
        if not fence:
            # sane_lists needs 4-space nesting; source uses 3
            m = re.match(r'^ {3}(?=[-*+] |\d+\. )', ln)
            if m:
                ln = ' ' + ln
            # drop horizontal-rule separators
            if ln.strip() == '---' and (not out or out[-1].strip() == ''):
                continue
            prev = out[-1] if out else ''
            starts_block = bool(LIST.match(ln)) or ln.lstrip().startswith('|')
            prev_same = bool(LIST.match(prev)) or prev.lstrip().startswith('|')
            if starts_block and prev.strip() and not prev_same and not prev.startswith('#'):
                out.append('')
        out.append(ln)
    return '\n'.join(out)

body_md = normalize(body_md)
title_line = src.splitlines()[0].lstrip('# ').strip()
intro = "Condensed, information-dense notes covering the full breadth of system design fundamentals needed for interviews."

html_body = markdown.markdown(
    body_md,
    extensions=['tables', 'fenced_code', 'toc', 'attr_list', 'sane_lists'],
    extension_configs={'toc': {'toc_depth': '2-2'}},
)

CSS = """
@page { size: A4; margin: 16mm 15mm 18mm 15mm;
  @bottom-center { content: counter(page); } }
* { box-sizing: border-box; }
body { font-family: "DejaVu Sans", "Liberation Sans", Arial, sans-serif;
  font-size: 9.6pt; line-height: 1.5; color: #1b1f24; margin: 0; }
.cover { height: 250mm; display: flex; flex-direction: column;
  justify-content: center; page-break-after: always; }
.cover .kicker { font-size: 10pt; letter-spacing: .22em; text-transform: uppercase;
  color: #8a6d3b; font-weight: 700; margin-bottom: 12mm; }
.cover h1 { font-size: 30pt; line-height: 1.15; margin: 0 0 8mm; color: #0d1117;
  border: none; padding: 0; letter-spacing: -0.5pt; }
.cover .rule { width: 34mm; height: 3px; background: #b8860b; margin-bottom: 8mm; }
.cover .sub { font-size: 12pt; color: #4a5058; max-width: 120mm; line-height: 1.6; }
h2 { font-size: 15pt; margin: 11mm 0 3.5mm; padding-bottom: 2mm; color: #0d1117;
  border-bottom: 2px solid #b8860b; page-break-after: avoid; letter-spacing: -0.2pt; }
h3 { font-size: 11.5pt; margin: 6mm 0 2mm; color: #24292f; page-break-after: avoid; }
h4 { font-size: 10pt; margin: 4mm 0 1.5mm; color: #3d444d; page-break-after: avoid; }
p { margin: 0 0 2.6mm; orphans: 2; widows: 2; }
ul, ol { margin: 0 0 3mm; padding-left: 5.5mm; }
li { margin-bottom: 1.1mm; }
li > ul, li > ol { margin-top: 1.1mm; }
strong { color: #0d1117; font-weight: 700; }
code { font-family: "DejaVu Sans Mono", "Liberation Mono", monospace; font-size: 8.4pt;
  background: #f2f4f7; padding: 0.4mm 1.2mm; border-radius: 2px; color: #a13b1f; }
pre { background: #f7f8fa; border: 1px solid #e2e5ea; border-left: 3px solid #b8860b;
  border-radius: 3px; padding: 3mm 3.5mm; overflow-x: auto; margin: 0 0 3.5mm;
  page-break-inside: avoid; }
pre code { background: none; padding: 0; color: #24292f; font-size: 8pt; line-height: 1.45; }
table { border-collapse: collapse; width: 100%; margin: 0 0 4mm; font-size: 8.7pt;
  page-break-inside: avoid; }
th { background: #eef1f5; text-align: left; font-weight: 700; color: #0d1117;
  border: 1px solid #ccd2da; padding: 1.8mm 2.2mm; }
td { border: 1px solid #dde1e7; padding: 1.6mm 2.2mm; vertical-align: top; }
tr:nth-child(even) td { background: #fafbfc; }
blockquote { margin: 0 0 3.5mm; padding: 2mm 0 2mm 4mm; border-left: 3px solid #b8860b;
  background: #fdfaf3; color: #4a4034; font-style: italic; }
blockquote p { margin: 0; }
hr { border: none; border-top: 1px solid #e2e5ea; margin: 7mm 0; }
a { color: #0d1117; text-decoration: none; }
"""

out = f"""<!doctype html><html><head><meta charset="utf-8">
<title>{title_line}</title><style>{CSS}</style></head><body>
<div class="cover">
  <div class="kicker">Interview Reference</div>
  <h1>{title_line}</h1>
  <div class="rule"></div>
  <div class="sub">{intro}</div>
</div>
{html_body}
</body></html>"""

pathlib.Path('/tmp/claude-0/-home-user-nityamittal/78dfcd0b-45fe-50db-910b-d06ac91e04b2/scratchpad/notes.html').write_text(out)
print("html written", len(out))
