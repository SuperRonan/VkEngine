import io

def stringToInt(s : str) -> int:
	if len(s) >= 2 and s[0:2] == "0x":
			return int(s, 16)
	return int(s)

file = open("formats.txt", "rt")

lines = file.readlines()

lines = lines

res = []

i = 0

for line in lines:
	
	(vk_format_string, eq, vk_format_number) = line.split(" ")
	vk_format_number = stringToInt(vk_format_number[0:len(vk_format_number) - 2])
	

	if vk_format_number > 1000:
		
		print("{%s, %d}," % (vk_format_string, i))
		i += 1

	
