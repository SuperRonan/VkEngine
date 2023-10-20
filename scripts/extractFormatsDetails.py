import io

format_types = [
	"None",
	"UNORM",
	"SNORM",
	"USCALED",
	"SSCALED",
	"UINT",
	"SINT",
	"SRGB",
	"UFLOAT",
	"SFLOAT",
]

def parseDecNumber(s : str, i : int) -> tuple[str, int]:
	res = ""
	
	while i < len(s) and ord(s[i]) >= ord('0') and ord(s[i]) <= ord('9'):
		res += s[i]
		i = i + 1

	return (res, i)

def parseComponentsString(comps : str) -> list[tuple[str, int]]:
	res = []

	i = 0
	while i < len(comps):
		channel_name = comps[i]
		i = i + 1
		channel_count, i = parseDecNumber(comps, i)
		channel_count = int(channel_count)
		res.append((channel_name, channel_count))

	return res

def stringToInt(s : str) -> int:
	if len(s) >= 2 and s[0:2] == "0x":
			return int(s, 16)
	return int(s)

class DetailedFormat:

	def __init__(self, format_str : str) -> None:
		
		self.knows_details = False
		self.color_aspect = False
		self.depth_aspect = False
		self.stencil_aspect = False
		self.vk_format_index = 0
		self.vk_format = ""
		self.color_format_type = ""
		self.depth_format_type = ""
		self.stencil_format_type = ""
		self.channels = 0
		self.bits_per_component = 0
		self.depth_bits = 0
		self.stencil_bits = 0
		self.swizzle = ""
		self.pack_bits = 0
			
		# for example
		# format_str = "VK_FORMAT_R4G4B4A4_UNORM_PACK16 = 2,"

		(vk_format_string, eq, index_str) = format_str.split(" ")
		index_str = index_str[0:len(index_str) - 2]

		self.vk_format = vk_format_string
		self.vk_format_index = stringToInt(index_str)

		split_format = vk_format_string.split("_")

		# (min_included, max_included)
		raw_formats_range = (1, 123)
		depth_stencil_formats_range = (124, 130)

		in_raw_range = self.vk_format_index >= raw_formats_range[0] and self.vk_format_index <= raw_formats_range[1]
		in_depth_stencil_range = self.vk_format_index >= depth_stencil_formats_range[0] and self.vk_format_index <= depth_stencil_formats_range[1]

		if in_raw_range:
			self.color_aspect = True
			self.color_format_type = split_format[3]
			
			pack_str = split_format[-1]
			if pack_str[0:4] == "PACK":
				self.pack_bits = int(pack_str[4:])				

			

			components = split_format[2]
			comps_list = parseComponentsString(components)
			
			comps_str = ""
			all_equal = True
			self.channels = len(comps_list)
			
			
			self.bits_per_component = [0] * 4
			for i in range(self.channels):
				self.bits_per_component[i] = comps_list[i][1]
				if i > 0:
					all_equal = all_equal and (self.bits_per_component[i] == self.bits_per_component[i - 1])
				comps_str += comps_list[i][0]
			if all_equal:
				self.bits_per_component = self.bits_per_component[0]
			self.swizzle = comps_str
			if self.vk_format == "VK_FORMAT_E5B9G9R9_UFLOAT_PACK32":
				self.channels = 3
				self.bits_per_component = [16, 16, 16, 0]
				self.is_reversed = True
			self.knows_details = True


		if in_depth_stencil_range:
			if self.vk_format == "VK_FORMAT_D16_UNORM":
				self.depth_aspect = True
				self.depth_bits = 16
				self.depth_format_type = "UNORM"
				pass
			elif self.vk_format == "VK_FORMAT_X8_D24_UNORM_PACK32":  
				self.depth_aspect = True
				self.depth_bits = 24
				self.pack_bits = 32
				self.depth_format_type = "UNORM"
				pass
			elif self.vk_format == "VK_FORMAT_D32_SFLOAT":
				self.depth_aspect = True
				self.depth_bits = 32
				self.depth_format_type = "SFLOAT"
				pass
			elif self.vk_format == "VK_FORMAT_S8_UINT":
				self.stencil_aspect = True
				self.stencil_bits = 8
				self.stencil_format_type = "UINT"
				pass
			elif self.vk_format == "VK_FORMAT_D16_UNORM_S8_UINT":
				self.depth_aspect = True
				self.stencil_aspect = True
				self.depth_bits = 16
				self.stencil_bits = 8
				self.depth_format_type = "UNORM"
				self.stencil_format_type = "UINT"
				pass
			elif self.vk_format == "VK_FORMAT_D24_UNORM_S8_UINT":
				self.depth_aspect = True
				self.stencil_aspect = True
				self.depth_bits = 24
				self.stencil_bits = 8
				self.depth_format_type = "UNORM"
				self.stencil_format_type = "UINT"
				pass
			elif self.vk_format == "VK_FORMAT_D32_SFLOAT_S8_UINT":
				self.depth_aspect = True
				self.stencil_aspect = True
				self.depth_bits = 32
				self.stencil_bits = 8
				self.depth_format_type = "SFLOAT"
				self.stencil_format_type = "UINT"
				pass
			self.knows_details = self.depth_aspect or self.stencil_aspect
			if self.stencil_format_type == "":
				self.stencil_format_type = "None"
			if self.depth_format_type == "":
				self.depth_format_type = "None"


			
	

			

	def toCpp(self) -> str:
		res = ""
		if not self.knows_details:
			res = "DetailedVkFormat(%s)" % self.vk_format
		elif self.color_aspect:
			bits_per_comp_str = ""
			if type(self.bits_per_component) is list:
				bits_per_comp_str = "{%d, %d, %d, %d}" % tuple(self.bits_per_component)
			else:
				bits_per_comp_str = str(self.bits_per_component)
			res = "DetailedVkFormat(%s, Type::%s, %d, %s, Swizzle::%s, %d)" % (self.vk_format, self.color_format_type, self.channels, bits_per_comp_str, self.swizzle, self.pack_bits)
		elif self.depth_aspect or self.stencil_aspect:
			res = "DetailedVkFormat(%s, Type::%s, %d, Type::%s, %d, %d)" % (self.vk_format, self.depth_format_type, self.depth_bits, self.stencil_format_type, self.stencil_bits, self.pack_bits)
			pass
		else:
			res = "DetailedVkFormat(%s)" % self.vk_format
		return res

file = open("formats.txt", "rt")

lines = file.readlines()

lines = lines

res = []

for line in lines:
	
	detailed_format = DetailedFormat(line)

	print(detailed_format.toCpp() + ",")
