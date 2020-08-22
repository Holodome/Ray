vertices = []
triangles = []

filename = "data/untitled.obj"
with open(filename, "r") as file:
    lines = file.readlines()
    for line in lines:
        if line.startswith("v"):
            _, x, y, z = line.split()
            vertices.append(tuple(map(float, (x, z, y))))
        elif line.startswith("f"):
            _, a, b, c = line.split()
            ai = int(a.split("/")[0])
            bi = int(b.split("/")[0])
            ci = int(c.split("/")[0])
            
            triangle = (vertices[ai - 1], vertices[bi - 1], vertices[ci - 1])
            triangles.append(triangle)
            

print(f"""
    *dcount = {len(triangles)};    """)
for i, triangle in enumerate(triangles):
    print(f"""triangles[{i}] = (Triangle) {{ .mat_index = mat_index, 
    .vertex0 = vec3({",".join(map(str, triangle[0]))}),
    .vertex1 = vec3({",".join(map(str, triangle[1]))}),
    .vertex2 = vec3({",".join(map(str, triangle[2]))}),
    }};""")