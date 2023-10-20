import io


file = open("formats.txt", "rt")

lines = file.readlines()

res = []

for line in lines:
	format = line.split(" ")[0]
	format = format[10:]
	format = "\"" + format + "\","
	print(format)
