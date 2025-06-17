import sys

while True:
	addr = input()
	if addr.count("."):
		addr = [int(x, 16) for x in addr.split(".")]
		res = addr[0] << 39 | addr[1] << 30 | addr[2] << 21 | addr[3] << 12
		if res & (1 << 47):
			res |= 0xffff << 48
		print(f"0x{res:016x}")
	else:
		addr = int(addr, 16)
		res = [(addr >> 39) & 0x1ff, (addr >> 30) & 0x1ff, (addr >> 21) & 0x1ff, (addr >> 12) & 0x1ff]
		print(f"{res[0]:03x}.{res[1]:03x}.{res[2]:03x}.{res[3]:03x}")
