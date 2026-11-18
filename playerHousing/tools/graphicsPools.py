"""Prints the resource pools of EQGraphicsDX9.dll in a running RoF2 spare, the counts the DLL's own Pools line (0x1005aa60)
formats: per pool the sums of every node's +4 (what the pool holds) and +8 (what is in use), its nodes linked at +0x10.

Run with claudelink's environment, from claudelink/mcp:
  .venv/Scripts/python.exe <path>/graphicsPools.py <process id> [--zone]
The Zone pool runs to tens of thousands of nodes, minutes of reads through the link, so it is walked only with --zone.
"""
import argparse
import ctypes
import struct
from ctypes import wintypes

from claudeLinkMcp import cli, shellLink, spareLink

graphicsModule = 'EQGraphicsDX9.dll'
poolHoldersOffset = 0x1d0108
formatOffset = 0x138dd8
formatText = b'Pools: Temp: %d(%d).'
poolNames = ('Temp', 'Persistent', 'OnDemand', 'Zone')

kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
kernel32.OpenProcess.restype = wintypes.HANDLE
kernel32.OpenProcess.argtypes = (wintypes.DWORD, wintypes.BOOL, wintypes.DWORD)
kernel32.K32EnumProcessModulesEx.argtypes = (wintypes.HANDLE, ctypes.POINTER(ctypes.c_void_p), wintypes.DWORD, ctypes.POINTER(wintypes.DWORD), wintypes.DWORD)
kernel32.K32GetModuleBaseNameW.argtypes = (wintypes.HANDLE, ctypes.c_void_p, wintypes.LPWSTR, wintypes.DWORD)
kernel32.CloseHandle.argtypes = (wintypes.HANDLE,)


def moduleBase(processID, moduleName):
  process = kernel32.OpenProcess(0x0400 | 0x0010, False, processID)
  if not process:
    raise OSError(ctypes.get_last_error(), f'process {processID} could not be opened')
  try:
    modules = (ctypes.c_void_p * 1024)()
    needed = wintypes.DWORD()
    if not kernel32.K32EnumProcessModulesEx(process, modules, ctypes.sizeof(modules), ctypes.byref(needed), 0x01):
      raise OSError(ctypes.get_last_error(), f'the modules of process {processID} could not be listed')
    name = ctypes.create_unicode_buffer(260)
    for module in modules[:needed.value // ctypes.sizeof(ctypes.c_void_p)]:
      if kernel32.K32GetModuleBaseNameW(process, module, name, len(name)) and name.value.lower() == moduleName.lower():
        return module
  finally:
    kernel32.CloseHandle(process)
  raise LookupError(f'process {processID} has no {moduleName}')


def main():
  parser = argparse.ArgumentParser(description='the resource pools of EQGraphicsDX9.dll in a RoF2 spare')
  parser.add_argument('processID', type=int)
  parser.add_argument('--zone', action='store_true', help='walk the Zone pool too')
  arguments = parser.parse_args()
  base = moduleBase(arguments.processID, graphicsModule)
  session = shellLink.openSession(cli.side, arguments.processID)

  def read(address, size):
    return bytes.fromhex(spareLink.shaped(session.command('peek', hex(address), size))['hex'])

  try:
    if read(base + formatOffset, len(formatText)) != formatText:
      raise RuntimeError(f'{graphicsModule} at {base:#x} is not the RoF2 build this reads (no Pools line at +{formatOffset:#x})')
    holders = struct.unpack('<4I', read(base + poolHoldersOffset, 16))
    lines = []
    for name, holder in zip(poolNames, holders):
      if name == 'Zone' and not arguments.zone:
        continue
      node = struct.unpack('<I', read(holder, 4))[0]
      held = used = count = 0
      while node:
        _, size, inUse, _, node = struct.unpack('<5I', read(node, 20))
        held += size
        used += inUse
        count += 1
      lines.append(f'{name}: {held}({used}) in {count} nodes')
    print('Pools: ' + '. '.join(lines))
  finally:
    session.close()


if __name__ == '__main__':
  main()
