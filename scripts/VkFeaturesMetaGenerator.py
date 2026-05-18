
from common import *

class FeaturesReflectionGenerator(CommonBaseGenerator):

	def __init__(self):
		super().__init__()
		self.sTypes_dict : dict[str, vulkan_object.EnumField] = {}

	def getDefaultFileName(self):
		return None

	def structIsFeatures(self, struct : vulkan_object.Struct):
		if 'Features' not in struct.name:
			return False
		if struct.name == "VkPhysicalDeviceFeatures":
			return True
		if struct.name == "VkPhysicalDeviceFeatures2":
			return True
		if len(struct.members) <= 2:
			return False
		if not (MemberIs_sType(struct.members[0]) and MemberIs_pNext(struct.members[1])):
			return False
		remaining_members = struct.members[2:]
		correct_members = [member.type == 'VkBool32' for member in remaining_members]
		return all(correct_members)

	def collectAllStructs(self):
		res = [struct for struct in self.vk.structs.values() if self.structIsFeatures(struct)]
		return res

	def fixupStructs(self, structs : list[vulkan_object.Struct], sort=False):
		structs = structs.copy()
		VkPhysicalDeviceFeatures = [s for s in structs if s.name == 'VkPhysicalDeviceFeatures']
		VkPhysicalDeviceFeatures2 = [s for s in structs if s.name == 'VkPhysicalDeviceFeatures2']
		assert(len(VkPhysicalDeviceFeatures) == len(VkPhysicalDeviceFeatures2))
		if len(VkPhysicalDeviceFeatures) == 0:
			return structs
		VkPhysicalDeviceFeatures = VkPhysicalDeviceFeatures[0]
		VkPhysicalDeviceFeatures2 = VkPhysicalDeviceFeatures2[0]
		structs.pop(structs.index(VkPhysicalDeviceFeatures))
		VkPhysicalDeviceFeatures2.members.pop()
		VkPhysicalDeviceFeatures2.members.extend(VkPhysicalDeviceFeatures.members)

		if sort:
			key = lambda struct: self.sTypes_dict[struct.sType].value
			structs.sort(key=key)

		return structs

	def gatherStatistics(self, structs : list[vulkan_object.Struct]):
		maxLabelLen = 0
		countLabelLen = 0
		labelCount = 0
		for struct in structs:
			labelCount = labelCount + len(struct.members) - 2
			for member in struct.members[2:]:
				maxLabelLen = max(maxLabelLen, len(member.name))
				countLabelLen = countLabelLen + len(member.name)
		return {
			'maxLabelLen' : maxLabelLen,
			'countLabelLen' : countLabelLen,
			'labelCount' : labelCount,
		}

	def generate(self):
		sTypes_enum = self.vk.enums['VkStructureType']
		sTypes_dict = {e.name : e for e in sTypes_enum.fields}
		self.sTypes_dict = sTypes_dict
		structs_zero = self.collectAllStructs()
		structs2 = self.fixupStructs(structs_zero, sort=True)
		stats = self.gatherStatistics(structs2)

		
		# print(stats)
		# for struct in structs:
		# 	print(struct.name)

		maxLabelLen = stats['maxLabelLen']

		constants = CStyleWriter(pragma_once=True, generator=__file__)

		constants.open("namespace vkl::meta::features")
		constants.append(DEFINE_CONSTINIT, indent='')
		constants.append(f'static {CONSTINIT} const size_t MaxLabelLength = {maxLabelLen};')
		constants.append(f'static {CONSTINIT} const size_t LabelCount = {stats['labelCount']};')
		constants.append(f'static {CONSTINIT} const size_t CountLabelLen = {stats['countLabelLen']};')
		constants.append(f'static {CONSTINIT} const size_t StructCount = {len(structs2)};')
		constants.close()
		self.writeHeader(str(constants), "VulkanFeaturesMetaConstants.hpp")

		source = CStyleWriter(pragma_once=False, generator=__file__)
		source.append('#include <vkl/VkObjects/VulkanFeaturesMeta.hpp>')
		source.open("namespace vkl::meta")

		source.append('using Index = FeaturesMeta::Index;')
		source.open('const char* const FeaturesMeta::_g_features_labels_buffer = ', opening=None)
		for struct in structs2:
			# no pragma guard needed here
			for member in struct.members[2:]:
				label = member.name
				zeros_to_fill = 1 + maxLabelLen - len(label)
				chars = label + '\\0' * zeros_to_fill
				source.append(f'"{chars}"')
		source.close(';')

		# source.open('''Index FeaturesMeta::Flatten_sType(VkStructureType sType)''')
		# source.open('switch(sType)')
		# counter = 0
		# for struct in structs[1:]:
		# 	if struct.protect is None:
		# 		case = f'{struct.sType}'
		# 		comment = ''
		# 	else:
		# 		case = f'{sTypes_dict[struct.sType].value}'
		# 		comment = ' // ' + struct.sType
		# 	source.append(f'case {case}: return {counter}; break; {comment}')
		# 	counter = counter + 1
		# source.close()
		# source.append('return -1;')
		# source.close('} // FeaturesMeta::Flatten_sType()')

		source.append(f'const size_t FeaturesMeta::_g_structs_count = {len(structs2)};')
		source.open('const Index _g_features_metas_storage[] = ')
		counter = 0
		for struct in structs2:
			source.append(f'Index({counter}), // {struct.name}')
			counter = counter + len(struct.members) - 2
		source.append(f'Index({counter}),')
		source.close('};')
		source.append('const Index * const FeaturesMeta::_g_features_metas = _g_features_metas_storage;')

		source.open('const VkStructureType _g_features_sTypes_storage[] = ')
		for struct in structs2:
			if struct.protect is None:
				value = struct.sType
				comment = ''
			else:
				value = f'VkStructureType({sTypes_dict[struct.sType].value})'
				comment = ' // ' + struct.sType
			source.append(f'{value},{comment}')
		source.close('};')
		source.append('const VkStructureType * const FeaturesMeta::_g_features_sTypes = _g_features_sTypes_storage;')

		source.close("} // namespace vkl::meta")
		self.writeSource(str(source), "VulkanFeaturesMeta.cpp")

		header = CStyleWriter(pragma_once=True, generator=__file__)
		header.append('#include <vkl/VkObjects/VulkanFeaturesMetaTemplate.hpp>')
		header.open("namespace vkl::meta")

		counter = 0
		guard = PlatformGuardHelper()
		for struct in structs2:
			header.append(guard.add_guard(struct.protect), indent='')
			header.append(f'template <> consteval uint FeaturesMeta::GetVkFeaturesStructFlatIndex<{struct.name}>(){{return {counter};}}')
			counter = counter + 1
		header.append(guard.add_guard(None), indent='')

		header.close("} // namespace vkl::meta")
		self.writeHeader(str(header), "VulkanFeaturesMeta.hpp")

if __name__ == "__main__":
	RunGenerator(FeaturesReflectionGenerator)