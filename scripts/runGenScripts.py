from common import *

from EnumMetaGenerator import EnumMetaGenerator
from TypeHelperGenerator import TypeHelperGenerator
from VkFeaturesMetaGenerator import FeaturesReflectionGenerator

# This script / these scripts are not very optimized
# The same vulkan registry is (re-)built from scratch every time

RunGenerator(EnumMetaGenerator)

RunGenerator(TypeHelperGenerator)

RunGenerator(FeaturesReflectionGenerator)