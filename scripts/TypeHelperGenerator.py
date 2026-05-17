
from common import *

class TypeHelperGenerator(BaseGenerator):

	def __init__(self):
		super().__init__()

	def generate(self):
		out = CStyleWriter(pragma_once=True, generator=__file__)
		out.append('#include <vulkan/vulkan.h>')
		out.append(DEFINE_CONSTEVAL, indent='')
		out.open('''namespace vku2// Avoid conflict with current definitions in vku''')

		out.append(f'template <class VkHandle> static {CONSTEVAL} VkObjectType GetObjectType();')
		out.append('')
		guard = PlatformGuardHelper()
		for handle in self.vk.handles.values():
			out.append(guard.add_guard(handle.protect), indent='')
			out.append(f'template <> static {CONSTEVAL} VkObjectType GetObjectType<{handle.name}>() {{return {handle.type}; }}')
		out.append(guard.add_guard(None), indent='')

		out.append('', indent='')

		out.append(f'template <class VkStruct> static {CONSTEVAL} VkStructureType GetSType();')
		for struct in self.vk.structs.values():
			if struct.sType is not None:
				out.append(guard.add_guard(struct.protect), indent='')
				out.append(f'template <> static {CONSTEVAL} VkStructureType GetSType<{struct.name}>() {{return {struct.sType}; }}')
		out.append(guard.add_guard(None), indent='')

		out.close('} // namespace vku2')
		self.write(str(out))

	def getDefaultFileName(self):
		return "VulkanTypeStructHelper.hpp"
