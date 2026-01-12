import sys
from pathlib import Path

root_dir = Path(__file__).parent.parent
sys.path.append(str(root_dir))
scripts_dir = root_dir.joinpath("ext").joinpath("VulkanDocs").joinpath("scripts")
sys.path.append(str(scripts_dir))

from ext.VulkanDocs.scripts.base_generator import (BaseGenerator, BaseGeneratorOptions, SetTargetApiName, SetMergedApiNames)
from ext.VulkanDocs.scripts.reg import Registry

from ext.VulkanDocs.scripts import vulkan_object

from xml.etree import ElementTree

# Borrowed from Vulkan Utility Library
class PlatformGuardHelper():
	"""Used to elide platform guards together, so redundant #endif then #ifdefs are removed
	Note - be sure to call add_guard(None) when done to add a trailing #endif if needed
	"""
	def __init__(self):
		self.current_guard = None

	def add_guard(self, guard, extra_newline = False):
		out = []
		if self.current_guard != guard and self.current_guard is not None:
			out.append(f'#endif  // {self.current_guard}')
		if extra_newline:
			out.append('')
		if self.current_guard != guard and guard is not None:
			out.append(f'#ifdef {guard}')
		self.current_guard = guard
		return out

class CStyleWriter:

	def __init__(self, indentation = '    ', open_block_inline = False):
		self.out = []
		self.indent = 0
		self.indentation = indentation
		self.open_block_inline = open_block_inline

	def inc(self, count : int = 1):
		self.indent += count
	
	def dec(self, count : int = 1):
		self.inc(-count)

	def append(self, block : str | list[str], split = True, indent = None):
		if type(block) is list:
			for statement in block:
				self.append(statement, split=split, indent=indent)
			return
		elif type(block) is str:
			if split:
				self.append(block.split('\n'), split=False, indent=indent)
			else:
				if len(block) == 0:
					self.out.append('')
				else:
					if indent is None:
						indent = self.indentation * self.indent
					self.out.append(f'{indent}{block}')
		elif type(block) is CStyleWriter:
			self.append(block.out, split, indent=indent)
		else:
			raise NotImplementedError(f'Writer::append expects a str or list[str], not {str(type(block))}!')
	
	def open(self, block, opening = '{'):
		self.append(block)
		if opening is not None:
			self.append(opening)
		self.inc()

	def close(self, closing = '}'):
		self.dec()
		if closing is not None:
			self.append(closing)
		pass

	def __str__(self):
		return '\n'.join(self.out)

def ExtractExtensionTag(ext : vulkan_object.Extension):
	res = ext.name.split('_')[1]
	return res

def DefaultRegistryPath():
	return root_dir.joinpath("ext").joinpath("VulkanDocs").joinpath("xml").joinpath("vk.xml")

def DefaultOutDir():
	return root_dir.joinpath("include").joinpath("vkl").joinpath("Generated")

def RunGenerator(GeneratorType, target=None, registry=DefaultRegistryPath(), outDirectory=DefaultOutDir()):
	SetTargetApiName("vulkan")
	SetMergedApiNames(None)

	gen = GeneratorType()

	if target is None:
		target = gen.getDefaultFileName()

	options = BaseGeneratorOptions(customFileName = target, customDirectory = outDirectory)

	reg = Registry(gen, options)

	# Parse the specified registry XML into an ElementTree object
	tree = ElementTree.parse(registry)

	# Load the XML tree into the registry object
	reg.loadElementTree(tree)

	# Finally, use the output generator to create the requested target
	reg.apiGen()