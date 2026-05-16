
from common import *

class FeaturesReflectionGenerator(BaseGenerator):

	def __init__(self):
		BaseGenerator().__init__(self)
	
	def collectAllStructs(self):
		for struct in self.vk.extensions.values():

	def generate(self):
		self.collectAllStructs()
		out = CStyleWriter()

		self.write(str(out))



def main(agrv):
	SetTargetApiName("vulkan")
	SetMergedApiNames(None)
	target = "VulkanFeaturesReflection.hpp"
	registry = root_dir.joinpath("ext").joinpath("VulkanDocs").joinpath("xml").joinpath("vk.xml")
	outDirectory = root_dir.joinpath("include").joinpath("vkl").joinpath("generated")
	generator = FeaturesReflectionGenerator
	gen = generator()

	options = BaseGeneratorOptions(customFileName = target, customDirectory = outDirectory)

	reg = Registry(gen, options)

	# Parse the specified registry XML into an ElementTree object
	tree = ElementTree.parse(registry)

	# Load the XML tree into the registry object
	reg.loadElementTree(tree)

	# Finally, use the output generator to create the requested target
	reg.apiGen()

if __name__ == "__main__":
	main(sys.argv[1:])