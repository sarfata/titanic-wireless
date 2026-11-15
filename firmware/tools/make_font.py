"""Rebuild the bundled GFX font from Google Fonts' OFL Patrick Hand font.
Requires Pillow; retains the original font license in licenses/.
"""
from pathlib import Path
import urllib.request
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1]
fontpath=root/'licenses'/'PatrickHand-Regular.ttf'
for path,url in [(fontpath,'https://raw.githubusercontent.com/google/fonts/main/ofl/patrickhand/PatrickHand-Regular.ttf'),(root/'licenses'/'PatrickHand-OFL.txt','https://raw.githubusercontent.com/google/fonts/main/ofl/patrickhand/OFL.txt')]:
 if not path.exists():path.write_bytes(urllib.request.urlopen(url,timeout=25).read())
font=ImageFont.truetype(str(fontpath),30)
bitmap=[];glyphs=[]
for n in range(32,127):
 c=chr(n);x0,y0,x1,y1=font.getbbox(c,anchor='ls');w,h=x1-x0,y1-y0
 glyphs.append((len(bitmap),w,h,round(font.getlength(c)),x0,y0))
 if not w or not h:continue
 im=Image.new('L',(w,h));ImageDraw.Draw(im).text((-x0,-y0),c,font=font,fill=255,anchor='ls')
 bits=[v>=100 for v in im.getdata()]
 for i in range(0,len(bits),8):
  value=0
  for j,bit in enumerate(bits[i:i+8]):value|=int(bit)<<(7-j)
  bitmap.append(value)
lines=['#pragma once','// Patrick Hand by Patrick Wagesreiter, SIL OFL 1.1; see licenses/.', '#include <Adafruit_GFX.h>','const uint8_t PatrickHand30Bitmaps[] PROGMEM = {']
for i in range(0,len(bitmap),20):lines.append('  '+','.join(f'0x{x:02x}' for x in bitmap[i:i+20])+',')
lines+=['};','const GFXglyph PatrickHand30Glyphs[] PROGMEM = {']
lines+=['  {'+','.join(map(str,g))+'},' for g in glyphs]
lines+=['};','const GFXfont PatrickHand30 PROGMEM = {(uint8_t*)PatrickHand30Bitmaps,(GFXglyph*)PatrickHand30Glyphs,32,126,39};']
(root/'include'/'PatrickHand30.h').write_text('\n'.join(lines)+'\n')
print('Font generated:',len(bitmap),'bytes')
