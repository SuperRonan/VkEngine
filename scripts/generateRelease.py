

import sys
import os
import argparse
import shutil
import datetime

def HasExtension(filename : str, extensions : list[str] = []) -> bool:
	if extensions is None or len(extensions) == 0:
		return True
	ext = os.path.splitext(filename)[1]
	if len(ext) <= 1:
		return False
	ext = ext[1:]
	return ext in extensions

def ListFiles(directory : str, extensions : list[str] = [], just_filename : bool = True) -> list :
	res = [
		f for f in os.listdir(directory)
		if (os.path.isfile(os.path.join(directory, f)) and HasExtension(f, extensions))
	]
	if not just_filename:
		res = [os.path.join(directory, f) for f in res]
	return res


parser = argparse.ArgumentParser()

parser.add_argument('projects', help="List of projects to install", nargs="+", type=str)
parser.add_argument("-dst", "--destination", help="Destination directory", type=str, required=False)
parser.add_argument("-a", "--assets", help="Copy assets", type=int, default=1, required=False)

args = parser.parse_args()
#print(vars(args))

this_directory = os.path.dirname(os.path.abspath(__file__))
root = os.path.abspath(os.path.join(this_directory, os.pardir))

if args.destination is None:
	dst_dir = this_directory
else:
	dst_dir = args.destination + "/"

time = datetime.datetime.now()
dst_dir = os.path.abspath(os.path.join(dst_dir, f"Release-{time.year}-{time.month}-{time.day}"))

print(f"Making release: {dst_dir}")
print(f"From: {root}")

os.makedirs(dst_dir, exist_ok=True)

shutil.copy2(os.path.join(root, "LICENSE"), os.path.join(dst_dir, "LICENSE"))
shutil.copy2(os.path.join(root, "README.md"), os.path.join(dst_dir, "README.md"))

src_assets_path = os.path.join(root, "assets")
has_assets = args.assets != 0 and os.path.exists(src_assets_path)
if has_assets:
	print("Copying assets...", end="")
	shutil.copytree(src_assets_path, os.path.join(dst_dir, "assets"), dirs_exist_ok=True)
	print("Done!")

dst_bin = os.path.join(dst_dir, "bin")
dst_projects = os.path.join(dst_dir, "projects")
dst_shaders = os.path.join(dst_dir, "shaders")

os.makedirs(dst_bin, exist_ok=True)
os.makedirs(dst_shaders, exist_ok=True)
#os.makedirs(dst_projects, exist_ok=True)

projects = args.projects
src_exec_dir = os.path.join(root, "build/projects/Release")

dlls_to_copy = ListFiles(src_exec_dir, ["dll"])

shutil.copytree(os.path.join(root, "Shaders"), dst_shaders, copy_function=shutil.copy2, dirs_exist_ok=True)

for p in projects:
	exec_filename = p + ".exe"
	exec_filepath = os.path.join(src_exec_dir, exec_filename)
	has_executable = os.path.exists(exec_filepath)

	project_bin_dir = os.path.join(dst_bin, p)
	project_type = "Executable" if has_executable else "Library"

	print(f"Installing {project_type} {p}...", end=" ")

	p_shaders_dst = ""

	if has_executable:
		os.makedirs(project_bin_dir, exist_ok=True)
		shutil.copy2(exec_filepath, os.path.join(project_bin_dir, exec_filename))
		p_shaders_dst = os.path.join(project_bin_dir, "shaders")
		for dll in dlls_to_copy:
			full_dll_path = os.path.join(src_exec_dir, dll)
			shutil.copy2(full_dll_path, project_bin_dir)
		mp_file = ""
		mp_file += f"\"ShaderLib\" \"../../shaders/ShaderLib\"\n"
		mp_file += f"\"gen\" \"../../gen\"\n"
		if has_assets:
			mp_file += f"\"assets\" \"../../assets\"\n"
		if "RenderLib" in projects:
			mp_file += f"\"RenderLibShaders\" \"../../shaders\"\n"

		with open(os.path.join(project_bin_dir, "MountingPoints.txt"), 'w') as f:
			f.write(mp_file)
	else:
		p_shaders_dst = dst_shaders

	shutil.copytree(os.path.join(root, f"projects/{p}/Shaders"), p_shaders_dst, copy_function=shutil.copy2, dirs_exist_ok=True)

	print("Done!")
