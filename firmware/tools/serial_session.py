"""Send explicit bench commands to one selected board and record its responses.
Example: python tools/serial_session.py --port /dev/cu.usbmodem2101 --command status
"""
import argparse,time,serial
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--port',required=True)
p.add_argument('--command',action='append',default=[]);p.add_argument('--seconds',type=float,default=3)
p.add_argument('--interval',type=float,default=.4)
p.add_argument('--log');p.add_argument('--preserve-control-lines',action='store_true',help='Avoid deliberate DTR/RTS toggles while opening an already-running board');args=p.parse_args()
lines=[];started=time.monotonic()
def record(s):
 line=f'{time.monotonic()-started:7.2f}s {s}';print(line,flush=True);lines.append(line)
def drain(seconds):
 end=time.monotonic()+seconds
 while time.monotonic()<end:
  line=board.readline()
  if line:record(line.decode('utf-8',errors='replace').rstrip())
class KeepLinesSerial(serial.Serial):
 def _update_dtr_state(self): pass
 def _update_rts_state(self): pass
board=(KeepLinesSerial if args.preserve_control_lines else serial.Serial)();board.port=args.port;board.baudrate=115200;board.timeout=.15
board.dtr=False;board.rts=False;board.open()
try:
 drain(2) # allow CoreS3 startup if opening USB caused a reset
 for command in args.command:
  record('> '+command);board.write((command+'\n').encode());board.flush();drain(args.interval)
 drain(args.seconds)
finally:
 board.close()
 if args.log:
  target=Path(args.log);target.parent.mkdir(parents=True,exist_ok=True)
  target.write_text('\n'.join(lines)+'\n')
