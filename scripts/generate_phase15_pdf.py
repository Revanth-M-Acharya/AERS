#!/usr/bin/env python3
"""
SMART AEGIS - Phase 15 Hardware Assembly and Deployment Manual PDF Generator
Generates a publication-grade PDF from docs/PHASE_15_HARDWARE_ASSEMBLY_AND_DEPLOYMENT_MANUAL.md
using ReportLab Platypus.
"""

import os
import re
import sys
from reportlab.lib.pagesizes import letter
from reportlab.lib import colors
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle,
    Preformatted, KeepTogether, PageBreak, HRFlowable
)
from reportlab.pdfgen import canvas

# Page geometry
PAGE_WIDTH, PAGE_HEIGHT = letter
MARGIN = 36.0  # 0.5 in
USABLE_WIDTH = PAGE_WIDTH - 2 * MARGIN  # 540.0 pt
USABLE_HEIGHT = PAGE_HEIGHT - 2 * MARGIN

class NumberedCanvas(canvas.Canvas):
    """Two-pass canvas for professional headers, footers, and Page X of Y numbering."""
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._saved_page_states = []

    def showPage(self):
        self._saved_page_states.append(dict(self.__dict__))
        self._startPage()

    def save(self):
        num_pages = len(self._saved_page_states)
        for state in self._saved_page_states:
            self.__dict__.update(state)
            self.draw_decorations(num_pages)
            super().showPage()
        super().save()

    def draw_decorations(self, num_pages):
        page = self._pageNumber
        self.saveState()

        # Top Header (pages 2+)
        if page > 1:
            self.setFont("Helvetica-Bold", 7.5)
            self.setFillColor(colors.HexColor("#0A2540"))
            self.drawString(MARGIN, PAGE_HEIGHT - 24, "SMART AEGIS — Physical Hardware Assembly & Commissioning Handbook")
            self.setFont("Helvetica", 7.5)
            self.setFillColor(colors.HexColor("#64748B"))
            self.drawRightString(PAGE_WIDTH - MARGIN, PAGE_HEIGHT - 24, "DOC ID: AEGIS-MAN-HW-2026-PHASE15")
            self.setStrokeColor(colors.HexColor("#CBD5E1"))
            self.setLineWidth(0.6)
            self.line(MARGIN, PAGE_HEIGHT - 28, PAGE_WIDTH - MARGIN, PAGE_HEIGHT - 28)

        # Bottom Footer (all pages)
        self.setStrokeColor(colors.HexColor("#CBD5E1"))
        self.setLineWidth(0.6)
        self.line(MARGIN, 30, PAGE_WIDTH - MARGIN, 30)

        self.setFont("Helvetica-Bold", 7.0)
        self.setFillColor(colors.HexColor("#0A2540"))
        self.drawString(MARGIN, 20, "QNX NEUTRINO RTOS 8.0 CLUSTER")
        self.setFont("Helvetica", 7.0)
        self.setFillColor(colors.HexColor("#64748B"))
        self.drawString(MARGIN + 145, 20, "|  CONFIDENTIAL & PROPRIETARY  |  HW-REV-2.1")
        
        page_str = f"Page {page} of {num_pages}"
        self.setFont("Helvetica-Bold", 7.5)
        self.setFillColor(colors.HexColor("#0A2540"))
        self.drawRightString(PAGE_WIDTH - MARGIN, 20, page_str)

        self.restoreState()


def clean_markdown_inline(text):
    """Converts Markdown inline syntax into ReportLab compatible tags and cleans math/chars."""
    # LaTeX math replacements
    text = re.sub(r'\$4 \\times 0\.85\\text\{ A\} = 3\.40\\text\{ A\}\$', '4 × 0.85 A = 3.40 A', text)
    text = re.sub(r'\$4 \\times 2\.50\\text\{ A\} = 10\.00\\text\{ A\}\$', '4 × 2.50 A = 10.00 A', text)
    text = re.sub(r'\$R \\le 0\.1\\ \\Omega\$', 'R ≤ 0.1 Ω', text)
    text = re.sub(r'\$0\.5\\text\{ N\}\\cdot\\text\{m\}\$', '0.5 N·m', text)
    text = re.sub(r'\$\\le 12\\text\{V\}\$ DC', '≤ 12V DC', text)
    text = re.sub(r'\$< 55^\\circ\\text\{C\}\$', '< 55°C', text)
    text = text.replace(r'\le', '≤').replace(r'\ge', '≥').replace(r'\Omega', 'Ω')
    text = text.replace(r'\times', '×').replace(r'\cdot', '·').replace(r'^\circ\text{C}', '°C').replace(r'^\circ', '°')
    text = text.replace('$', '')  # remove stray math delimiters

    # Markdown links [text](#anchor) -> <u>text</u>
    text = re.sub(r'\[([^\]]+)\]\([^\)]+\)', r'<u>\1</u>', text)

    # Inline bold & code
    text = re.sub(r'`([^`]+)`', r'<font name="Courier" color="#0F172A"><b>\1</b></font>', text)
    text = re.sub(r'\*\*([^*]+)\*\*', r'<b>\1</b>', text)
    text = re.sub(r'(?<!\*)\*([^*]+)\*(?!\*)', r'<i>\1</i>', text)

    # Convert self-closing HTML
    text = text.replace('<br>', '<br/>').replace('<hr>', '<hr/>')

    # Sanitize XML tags for ReportLab paraparser
    allowed_pattern = r'(</?(?:b|i|u|font|super|sub)(?:\s+[^>]*)?>|<br/>|<hr/>)'
    parts = re.split(allowed_pattern, text)
    result = []
    for part in parts:
        if re.match(allowed_pattern, part):
            result.append(part)
        else:
            # Escape & (if not already entity), < and >
            part_esc = re.sub(r'&(?!(amp|lt|gt|quot|apos|#\d+);)', '&amp;', part)
            part_esc = part_esc.replace('<', '&lt;').replace('>', '&gt;')
            result.append(part_esc)
    return ''.join(result)


def build_pdf(md_path, pdf_path):
    print(f"Reading markdown from: {md_path}")
    with open(md_path, "r", encoding="utf-8") as f:
        md_text = f.read()

    # Prepare document
    doc = SimpleDocTemplate(
        pdf_path,
        pagesize=letter,
        leftMargin=MARGIN,
        rightMargin=MARGIN,
        topMargin=MARGIN,
        bottomMargin=MARGIN
    )

    styles = getSampleStyleSheet()

    # Custom styles
    title_style = ParagraphStyle(
        'DocTitle',
        fontName='Helvetica-Bold',
        fontSize=18,
        leading=22,
        textColor=colors.HexColor('#0A2540'),
        spaceAfter=4
    )
    subtitle_style = ParagraphStyle(
        'DocSubTitle',
        fontName='Helvetica-Bold',
        fontSize=10.5,
        leading=14,
        textColor=colors.HexColor('#1E3A8A'),
        spaceAfter=12
    )
    h1_style = ParagraphStyle(
        'Heading1_Custom',
        fontName='Helvetica-Bold',
        fontSize=12,
        leading=15,
        textColor=colors.HexColor('#0A2540'),
        spaceBefore=14,
        spaceAfter=6,
        keepWithNext=True
    )
    h2_style = ParagraphStyle(
        'Heading2_Custom',
        fontName='Helvetica-Bold',
        fontSize=10,
        leading=13,
        textColor=colors.HexColor('#1E3A8A'),
        spaceBefore=10,
        spaceAfter=4,
        keepWithNext=True
    )
    body_style = ParagraphStyle(
        'Body_Custom',
        fontName='Helvetica',
        fontSize=8.5,
        leading=11.5,
        textColor=colors.HexColor('#1E293B'),
        spaceAfter=5
    )
    bullet_style = ParagraphStyle(
        'Bullet_Custom',
        fontName='Helvetica',
        fontSize=8.5,
        leading=11.5,
        textColor=colors.HexColor('#1E293B'),
        leftIndent=14,
        bulletIndent=4,
        spaceAfter=3
    )
    toc_item_style = ParagraphStyle(
        'TOC_Item',
        fontName='Helvetica',
        fontSize=7.8,
        leading=10.2,
        textColor=colors.HexColor('#0F172A')
    )

    story = []

    # Parse markdown line by line
    lines = md_text.splitlines()
    i = 0
    total_lines = len(lines)

    in_first_metadata = False

    while i < total_lines:
        line = lines[i]

        # 1. Skip empty lines
        if not line.strip():
            i += 1
            continue

        # 2. Document Title
        if line.startswith("# SMART AEGIS (AERS) — Phase 15 Engineering Manual"):
            story.append(Paragraph(clean_markdown_inline("SMART AEGIS (AERS) — Phase 15 Engineering Manual"), title_style))
            i += 1
            continue

        # 3. Document Subtitle
        if line.startswith("## "):
            story.append(Paragraph(clean_markdown_inline(line[3:].strip()), subtitle_style))
            i += 1
            continue

        # 4. Check for Initial Metadata block
        if line.startswith("```") and i < 20:
            # Check if this is the metadata header block
            block_lines = []
            i += 1
            while i < total_lines and not lines[i].startswith("```"):
                block_lines.append(lines[i])
                i += 1
            i += 1  # skip closing ```
            
            # Format metadata card
            meta_rows = []
            for bl in block_lines:
                bl_clean = bl.strip()
                if not bl_clean or bl_clean.startswith("="):
                    continue
                parts = bl_clean.split(":", 1)
                if len(parts) == 2:
                    k, v = parts[0].strip(), parts[1].strip()
                    meta_rows.append([
                        Paragraph(f"<b>{k}</b>", ParagraphStyle('MK', fontName='Helvetica-Bold', fontSize=7.5, leading=9.5, textColor=colors.HexColor('#0A2540'))),
                        Paragraph(v, ParagraphStyle('MV', fontName='Helvetica', fontSize=7.5, leading=9.5, textColor=colors.HexColor('#1E293B')))
                    ])
            if meta_rows:
                t = Table(meta_rows, colWidths=[120, USABLE_WIDTH - 120])
                t.setStyle(TableStyle([
                    ('BACKGROUND', (0,0), (-1,-1), colors.HexColor('#F8FAFC')),
                    ('BOX', (0,0), (-1,-1), 0.75, colors.HexColor('#CBD5E1')),
                    ('INNERGRID', (0,0), (-1,-1), 0.5, colors.HexColor('#E2E8F0')),
                    ('TOPPADDING', (0,0), (-1,-1), 3),
                    ('BOTTOMPADDING', (0,0), (-1,-1), 3),
                    ('LEFTPADDING', (0,0), (-1,-1), 6),
                    ('RIGHTPADDING', (0,0), (-1,-1), 6),
                ]))
                story.append(t)
                story.append(Spacer(1, 8))
            continue

        # 5. Table of Contents
        if line.strip() == "### Table of Contents":
            story.append(Paragraph("Table of Contents", h1_style))
            i += 1
            # Collect TOC items
            toc_lines = []
            while i < total_lines and not lines[i].strip().startswith("---"):
                tl = lines[i].strip()
                if re.match(r'^\d+\.', tl):
                    toc_lines.append(tl)
                i += 1
            
            # Split TOC into 2 columns
            mid = (len(toc_lines) + 1) // 2
            col1 = toc_lines[:mid]
            col2 = toc_lines[mid:]
            toc_table_data = []
            for idx in range(mid):
                item1 = col1[idx] if idx < len(col1) else ""
                item2 = col2[idx] if idx < len(col2) else ""
                p1 = Paragraph(clean_markdown_inline(item1), toc_item_style) if item1 else Paragraph("", toc_item_style)
                p2 = Paragraph(clean_markdown_inline(item2), toc_item_style) if item2 else Paragraph("", toc_item_style)
                toc_table_data.append([p1, p2])
            
            if toc_table_data:
                half_w = USABLE_WIDTH / 2.0
                t_toc = Table(toc_table_data, colWidths=[half_w, half_w])
                t_toc.setStyle(TableStyle([
                    ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
                    ('TOPPADDING', (0,0), (-1,-1), 1.5),
                    ('BOTTOMPADDING', (0,0), (-1,-1), 1.5),
                    ('LEFTPADDING', (0,0), (-1,-1), 4),
                    ('RIGHTPADDING', (0,0), (-1,-1), 4),
                    ('BACKGROUND', (0,0), (-1,-1), colors.HexColor('#F8FAFC')),
                    ('BOX', (0,0), (-1,-1), 0.5, colors.HexColor('#CBD5E1')),
                    ('INNERGRID', (0,0), (-1,-1), 0.3, colors.HexColor('#E2E8F0')),
                ]))
                story.append(t_toc)
                story.append(Spacer(1, 10))
            continue

        # 6. Horizontal Rule
        if line.strip() == "---":
            story.append(Spacer(1, 4))
            story.append(HRFlowable(width="100%", thickness=0.5, color=colors.HexColor('#CBD5E1'), spaceBefore=2, spaceAfter=6))
            i += 1
            continue

        # 7. Headings (### -> H1, #### -> H2)
        if line.startswith("### "):
            heading_text = line[4:].strip()
            # If this is Section 1 (Executive Summary), start on a fresh page after the Cover & TOC
            if heading_text.startswith("1. Executive Summary"):
                story.append(PageBreak())
            story.append(Paragraph(clean_markdown_inline(heading_text), h1_style))
            i += 1
            continue

        if line.startswith("#### "):
            heading_text = line[5:].strip()
            story.append(Paragraph(clean_markdown_inline(heading_text), h2_style))
            i += 1
            continue

        # 8. Code blocks / ASCII diagrams
        if line.startswith("```"):
            code_lines = []
            i += 1
            while i < total_lines and not lines[i].startswith("```"):
                code_lines.append(lines[i])
                i += 1
            i += 1  # skip closing ```

            raw_code = "\n".join(code_lines)
            max_line_len = max([len(l) for l in code_lines]) if code_lines else 0
            num_code_lines = len(code_lines)

            # Dynamic font sizing to ensure wide ASCII tables fit exactly in USABLE_WIDTH
            if max_line_len <= 80:
                f_size = 6.2
                leading = 7.6
            elif max_line_len <= 100:
                f_size = 5.4
                leading = 6.6
            elif max_line_len <= 120:
                f_size = 4.8
                leading = 5.8
            else:
                # Up to 135 characters
                f_size = 4.3
                leading = 5.3

            c_style = ParagraphStyle(
                f'Code_{max_line_len}_{i}',
                fontName='Courier',
                fontSize=f_size,
                leading=leading,
                textColor=colors.HexColor('#0F172A'),
                backColor=colors.HexColor('#F8FAFC'),
                borderColor=colors.HexColor('#CBD5E1'),
                borderWidth=0.5,
                borderPadding=4,
                spaceBefore=4,
                spaceAfter=6
            )
            pre = Preformatted(raw_code, c_style)
            # If small code block (<= 30 lines), keep together to avoid ugly splits
            if num_code_lines <= 30:
                story.append(KeepTogether([pre]))
            else:
                story.append(pre)
            story.append(Spacer(1, 4))
            continue

        # 9. Markdown Tables (| ... |)
        if line.strip().startswith("|") and line.strip().endswith("|"):
            table_lines = []
            while i < total_lines and lines[i].strip().startswith("|") and lines[i].strip().endswith("|"):
                table_lines.append(lines[i].strip())
                i += 1

            if len(table_lines) >= 3:
                # Parse header, delimiter, and rows
                header_raw = [c.strip() for c in table_lines[0].split("|")[1:-1]]
                # row 1 is delimiter (| :--- | :---: | etc)
                data_rows_raw = [[c.strip() for c in tl.split("|")[1:-1]] for tl in table_lines[2:]]

                num_cols = len(header_raw)

                # Column width heuristic
                if num_cols == 6:
                    if "Subsystem" in header_raw[0]:
                        # BOM table
                        col_widths = [60, 110, 115, 30, 75, 150]
                    elif "Node Identifier" in header_raw[0]:
                        # Node Allocation table
                        col_widths = [80, 95, 80, 45, 100, 140]
                    elif "Subsystem Branch" in header_raw[0]:
                        # Power Budget table
                        col_widths = [150, 60, 80, 80, 65, 105]
                    else:
                        w = USABLE_WIDTH / num_cols
                        col_widths = [w] * num_cols
                elif num_cols == 4:
                    # Revision History table
                    col_widths = [65, 75, 260, 140]
                else:
                    w = USABLE_WIDTH / num_cols
                    col_widths = [w] * num_cols

                # Normalize to USABLE_WIDTH
                total_w = sum(col_widths)
                col_widths = [w * (USABLE_WIDTH / total_w) for w in col_widths]

                table_data = []

                # Header cells
                h_style = ParagraphStyle('TH', fontName='Helvetica-Bold', fontSize=7.5, leading=9.5, textColor=colors.white)
                table_data.append([Paragraph(clean_markdown_inline(hc), h_style) for hc in header_raw])

                # Data cells
                c_style_data = ParagraphStyle('TD', fontName='Helvetica', fontSize=7.0, leading=9.0, textColor=colors.HexColor('#1E293B'))
                for r in data_rows_raw:
                    # Pad row if columns mismatch
                    row_cells = []
                    for c_idx in range(num_cols):
                        val = r[c_idx] if c_idx < len(r) else ""
                        row_cells.append(Paragraph(clean_markdown_inline(val), c_style_data))
                    table_data.append(row_cells)

                t = Table(table_data, colWidths=col_widths)
                t.setStyle(TableStyle([
                    ('BACKGROUND', (0,0), (-1,0), colors.HexColor('#0A2540')),
                    ('TEXTCOLOR', (0,0), (-1,0), colors.white),
                    ('ALIGN', (0,0), (-1,-1), 'LEFT'),
                    ('VALIGN', (0,0), (-1,-1), 'TOP'),
                    ('TOPPADDING', (0,0), (-1,-1), 2.5),
                    ('BOTTOMPADDING', (0,0), (-1,-1), 2.5),
                    ('LEFTPADDING', (0,0), (-1,-1), 4),
                    ('RIGHTPADDING', (0,0), (-1,-1), 4),
                    ('BOX', (0,0), (-1,-1), 0.6, colors.HexColor('#0A2540')),
                    ('INNERGRID', (0,0), (-1,-1), 0.4, colors.HexColor('#CBD5E1')),
                    ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.HexColor('#FFFFFF'), colors.HexColor('#F8FAFC')]),
                ]))
                story.append(t)
                story.append(Spacer(1, 6))
            continue

        # 10. Unordered List Items (- ... or * ...)
        if line.strip().startswith("- ") or line.strip().startswith("* "):
            item_text = line.strip()[2:].strip()
            # Handle [ ] checkbox items if any
            if item_text.startswith("[ ] "):
                item_text = "&#9633; " + item_text[4:]
            elif item_text.startswith("[x] ") or item_text.startswith("[X] "):
                item_text = "&#9632; " + item_text[4:]
            story.append(Paragraph(f"&bull;&nbsp; {clean_markdown_inline(item_text)}", bullet_style))
            i += 1
            continue

        # 11. Numbered List Items (1. ...)
        m_num = re.match(r'^(\d+)\.\s+(.*)', line.strip())
        if m_num:
            num_val = m_num.group(1)
            item_text = m_num.group(2)
            story.append(Paragraph(f"<b>{num_val}.</b>&nbsp; {clean_markdown_inline(item_text)}", bullet_style))
            i += 1
            continue

        # 12. Regular Paragraph
        story.append(Paragraph(clean_markdown_inline(line.strip()), body_style))
        i += 1

    print(f"Building PDF canvas with {len(story)} flowables...")
    doc.build(story, canvasmaker=NumberedCanvas)
    print(f"PDF successfully generated: {pdf_path}")


if __name__ == "__main__":
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    md_file = os.path.join(base_dir, "docs", "PHASE_15_HARDWARE_ASSEMBLY_AND_DEPLOYMENT_MANUAL.md")
    pdf_file = os.path.join(base_dir, "docs", "PHASE_15_HARDWARE_ASSEMBLY_AND_DEPLOYMENT_MANUAL.pdf")

    if not os.path.exists(md_file):
        print(f"Error: Markdown file not found at {md_file}")
        sys.exit(1)

    build_pdf(md_file, pdf_file)
