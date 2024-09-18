import io


scalar_types = ["float", "int", "uint", "double"]

def GetScalarTypePrefix(scalar_type : str):
	res = ""
	if scalar_type in ["float", "float32_t"]:
		res = ""
	else:
		if scalar_type in ["float16_t", "half"]:
			res = "f16"
		else:
			res = scalar_type[0]
	return res

def GetVectorTypeName(dim : int, scalar_type : str):
	res = ""
	if dim == 1:
		res = scalar_type
	else:
		res = GetScalarTypePrefix(scalar_type) + "vec" + str(dim)
	return res

for i in [1, 2, 3, 4]:

	