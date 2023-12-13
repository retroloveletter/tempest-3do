'''
    Combine all objects into one single object before exporting.
    
    Use the following export settings When exporting obj from blender:
        1. Forward Axis +Y
        2. Up Axis +Z
        3. Don't export normals
        4. Don't export materials
        5. Don't apply modifiers
        6. Don't export UV coordinates

    Finally, run this python script pointing to the obj file exported from Blender.

    Vertices will be in CCW order.
'''

import sys 
import os 

# 2^16 for 3DO's 16.16 format
FRACBITS_16 = pow(2, 16)

# types 

class vertex:
    def __init__(self):
        self.x = 0
        self.y = 0
        self.z = 0

class polygon:
    def __init__(self):
        self.vertex_lut = []

class object:
    def __init__(self):
        self.vertices = []
        self.polygons = []       

# helpers

def write_data_value(fout, value):
    fout.write( int(value).to_bytes(4, byteorder='big', signed=True) )
    return 4

# start 

if len(sys.argv) == 1:
    print("missing obj file path")
    sys.exit()

obj_filepath = sys.argv[1]

print(f"Working file '{obj_filepath}'")

# parse file

obj = object()

with open(obj_filepath) as f:
    lines = f.readlines()

for line in lines:
    # vertex line
    if line[0] == 'v' and line[1] == ' ':
        vert = vertex()
        line = line.rstrip()
        parts = line.split(' ')
        vert.x = float(parts[1])
        vert.y = float(parts[3]) # Swap Y and Z
        vert.z = float(parts[2])
        obj.vertices.append(vert)

    # face line
    if line[0] == 'f' and line[1] == ' ':
        poly = polygon()
        line = line.rstrip()
        parts = line.split(' ')
        poly.vertex_lut.append(int(parts[1]) - 1)
        poly.vertex_lut.append(int(parts[2]) - 1)
        poly.vertex_lut.append(int(parts[3]) - 1)
        poly.vertex_lut.append(int(parts[4]) - 1)        
        obj.polygons.append(poly)
               
# create data file

print(f"{len(obj.vertices)} vertices in file")
print(f"{len(obj.polygons)} polygons in file")

if len(obj.polygons) > 0:
    bytes_written = 0

    bytes_expected = (len(obj.vertices) * (4 * 3)) + (len(obj.polygons) * (4 * 4)) + 8

    with open("obj_data", "wb") as file:
        # first write out the number of vertices
        bytes_written += write_data_value(file, len(obj.vertices))

        # next write out the number of polygons
        bytes_written += write_data_value(file, len(obj.polygons))
        
        # next write each vertex
        print("Verts")
        for i in range(len(obj.vertices)):
            print(f"{obj.vertices[i].x} {obj.vertices[i].y} {obj.vertices[i].z}")
            xfixed = obj.vertices[i].x * float(FRACBITS_16)
            yfixed = obj.vertices[i].y * float(FRACBITS_16)
            zfixed = obj.vertices[i].z * float(FRACBITS_16)
            bytes_written += write_data_value(file, xfixed)
            bytes_written += write_data_value(file, yfixed)
            bytes_written += write_data_value(file, zfixed)

        # next write each polygon
        print("Faces")
        for i in range(len(obj.polygons)):
            for j in range(4):
                print(f"{obj.polygons[i].vertex_lut[j]} ", end='')
                bytes_written += write_data_value(file, obj.polygons[i].vertex_lut[j])    
            print()            

    print(f"{bytes_written} bytes written")      

    if bytes_written != bytes_expected:
        print(f"Error - Bytes written / expected mismatch {bytes_written} / {bytes_expected}") 
        sys.exit()
else:    
    print("Error - No polygons found in model file")
    sys.exit()
