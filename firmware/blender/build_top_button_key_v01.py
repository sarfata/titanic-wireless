"""Print-ready prototype top button and optional Morse lever. Dimensions in mm.

User confirms the same 6 mm ribbed EC11 as the front control. The 17 mm tip
projection is carried over from the front (same encoder / nominal 3 mm wall),
not a new measurement of the installed top. Click stroke remains unmeasured.
"""
import bpy, bmesh, json, math, struct
from pathlib import Path
from mathutils import Vector, Matrix
from mathutils.bvhtree import BVHTree

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'outputs/top_button_key_v01'
OUT.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = .001
scene.unit_settings.length_unit = 'MILLIMETERS'

def mat(name, color, metal=0, rough=.4):
    m = bpy.data.materials.new(name); m.use_nodes = True
    p = m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value = (*color, 1)
    p.inputs['Metallic'].default_value = metal
    p.inputs['Roughness'].default_value = rough
    return m

black = mat('Black button PLA', (.023, .029, .033))
gold = mat('Gold lever PLA', (.60, .34, .075), .65, .33)
steel = mat('Existing M3 hardware - reference only', (.30, .32, .34), .8)
wood = mat('Enclosure top - reference only', (.13, .055, .022), 0, .65)

def box(name, loc, size, bevel=0):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    o = bpy.context.object; o.name = name; o.dimensions = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel:
        m = o.modifiers.new('Rounded edges', 'BEVEL'); m.width = bevel; m.segments = 4
        bpy.ops.object.modifier_apply(modifier=m.name)
    return o

def cyl(name, loc, r, h, axis='Z', n=128):
    bpy.ops.mesh.primitive_cylinder_add(vertices=n, radius=r, depth=h, location=loc)
    o = bpy.context.object; o.name = name
    if axis == 'Y': o.rotation_euler.x = math.pi/2
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    return o

def boolean(o, c, op='DIFFERENCE'):
    m = o.modifiers.new(op, 'BOOLEAN'); m.solver = 'MANIFOLD'; m.operation = op; m.object = c
    bpy.context.view_layer.objects.active = o
    bpy.ops.object.modifier_apply(modifier=m.name)
    bpy.data.objects.remove(c, do_unlink=True)

def lathe(name, profile):
    n = 192; v = []; rings = []
    for r, z in profile:
        if r == 0:
            rings.append([len(v)]); v.append((0, 0, z))
        else:
            rings.append(list(range(len(v), len(v)+n)))
            v += [(r*math.cos(i*math.tau/n), r*math.sin(i*math.tau/n), z) for i in range(n)]
    f = []
    if len(rings[0]) > 1: f.append(tuple(reversed(rings[0])))
    if len(rings[-1]) > 1: f.append(tuple(rings[-1]))
    for a, b in zip(rings, rings[1:]):
        for i in range(n):
            j = (i+1) % n
            if len(a) == 1: f.append((a[0], b[j], b[i]))
            elif len(b) == 1: f.append((a[i], a[j], b[0]))
            else: f.append((a[i], a[j], b[j], b[i]))
    data = bpy.data.meshes.new(name); data.from_pydata(v, [], f); data.update()
    o = bpy.data.objects.new(name, data); scene.collection.objects.link(o)
    return o

def finish(o, name, material):
    # Apply all transforms so the exported coordinates equal the inspected mesh.
    bpy.ops.object.select_all(action='DESELECT'); o.select_set(True)
    bpy.context.view_layer.objects.active = o
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    bm = bmesh.new(); bm.from_mesh(o.data)
    bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=.00001)
    bmesh.ops.dissolve_degenerate(bm, edges=list(bm.edges), dist=.00001)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    remaining = set(bm.verts); count = 0
    while remaining:
        count += 1; stack = [remaining.pop()]
        while stack:
            for e in stack.pop().link_edges:
                for v in e.verts:
                    if v in remaining: remaining.remove(v); stack.append(v)
    bounds = [[min(v.co[i] for v in bm.verts), max(v.co[i] for v in bm.verts)] for i in range(3)]
    report = dict(nonmanifold_edges=sum(not e.is_manifold for e in bm.edges),
                  degenerate_faces=sum(f.calc_area()<1e-9 for f in bm.faces),
                  connected_components=count, volume_mm3=bm.calc_volume(signed=True),
                  bounds_mm=bounds, dimensions_mm=[b-a for a,b in bounds], physical_fit_verified=False)
    assert report['nonmanifold_edges']==0 and report['degenerate_faces']==0 and count==1 and report['volume_mm3']>0, (name, report)
    assert abs(bounds[2][0]) < .001, (name, bounds)
    bm.to_mesh(o.data); bm.free(); o.data.update()
    o.name = name; o.data.materials.clear(); o.data.materials.append(material)
    for p in o.data.polygons: p.material_index = 0
    o.data.calc_loop_triangles()
    with (OUT/(name+'.stl')).open('wb') as f:
        f.write(('Titanic top key prototype, mm: '+name).encode()[:80].ljust(80,b'\0'))
        f.write(struct.pack('<I', len(o.data.loop_triangles)))
        for t in o.data.loop_triangles:
            p = [o.data.vertices[i].co for i in t.vertices]
            n = (p[1]-p[0]).cross(p[2]-p[0]).normalized()
            f.write(struct.pack('<12fH', *n, *p[0], *p[1], *p[2], 0))
    return report

def span(o, origin, direction):
    bm = bmesh.new(); bm.from_mesh(o.data); tree = BVHTree.FromBMesh(bm)
    a = tree.ray_cast(Vector(origin), Vector(direction))[0]
    b = tree.ray_cast(Vector(origin), -Vector(direction))[0]; bm.free()
    assert a is not None and b is not None
    return (a-b).length

# Smooth mushroom head: flat finger face on bed, socket pointing up.
head_profile = [(12,0), (12.35,.35), (12.7,.7), (12.92,1.05), (13,1.5),
                (13,2.0), (12.9,2.5), (12.6,3), (11.7,3.7), (9,5.2), (8,6)]
button = lathe('Standalone mushroom', head_profile+[(8,18.4),(7.4,19)])
boolean(button, cyl('6mm push socket', (0,0,11.5), 3, 17))  # floor Z3, open at19
boolean(button, cyl('Nut recess', (0,0,17.25), 6.5, 7.5))  # floor13.5
bpy.ops.mesh.primitive_cone_add(vertices=128, radius1=3, radius2=3.6, depth=.6, location=(0,0,13.2))
boolean(button, bpy.context.object)
reports = {}
reports['01_Top_mushroom_button_6mm'] = finish(button, '01_Top_mushroom_button_6mm', black)
assert abs(span(button,(0,0,7),(1,0,0))-6)<.01
assert abs(span(button,(0,0,15),(1,0,0))-13)<.01

# Base assembled on box-top Z0. Shaft centre X30, pivot X0.
base = box('Glue-on base', (34,0,1.5), (84,22,3), .7)
boolean(base, cyl('Clear mounted encoder nut', (30,0,1.5), 7.5, 8))
for name,y,thickness in [('Near cheek',-3.8,3),('Far cheek',4.55,4.5)]:
    cheek = box(name, (0,y,13.7), (12,thickness,24.4), .5)
    boolean(cheek, cyl('Axle through', (0,y,19),1.7,12,'Y'))
    if name.startswith('Far'):
        boolean(cheek, cyl('H3 insert from outside', (0,5.3,19),2.2,3.0,'Y'))
    boolean(base, cheek, 'UNION')
# A short bridge over the4.6mm hinge slot limits upward swing, retaining the
# loose shaft cap. The lever slides in from its finger end before axle insertion.
roof = box('Hinge lift stop', (0,.75,25.125), (12,12.1,1.55), .15)
boolean(base, roof, 'UNION')
post = box('Adjustable stop landing', (43,0,9.5),(8,10,15),.6)
boolean(base, post, 'UNION')  # post top17; beam underside19 at rest
reports['02_Key_glue_base'] = finish(base,'02_Key_glue_base',black)

# Lever top is one plane at Z24: print upside down, no unsupported beam.
lever = box('Lever beam',(35,0,21.5),(68,8,5),.4)  # X1..69
lug = box('Pivot tongue',(1,0,19),(12,4,10),.4)   # X-5..7
boolean(lever,lug,'UNION')
for y in [-5,5]:
    boolean(lever,box('Hinge side clearance',(-1,y,20),(18,6,16)))
boolean(lever,cyl('Pivot clearance',(0,0,19),1.7,12,'Y'))
boolean(lever,cyl('Stop screw through',(43,0,21.5),1.7,12))
boolean(lever,cyl('Stop H3 insert',(43,0,22.5),2.2,3.0))
boolean(lever,cyl('Finger button M3 clearance',(64,0,21.5),1.7,12))
boolean(lever,cyl('Finger screw head recess',(64,0,19.6),3,1.2))
# Preserve assembly transform for render; printable top lies at Z0.
flip = Matrix.Translation((0,0,24)) @ Matrix.Rotation(math.pi,4,'X')
lever.data.transform(lever.matrix_world); lever.matrix_world = Matrix.Identity(4)
lever.data.transform(flip)
reports['03_Key_lever'] = finish(lever,'03_Key_lever',gold)

# Separate mushroom finger pad, held by M3x8 from below the lever.
finger = lathe('Key finger button',head_profile+[(8,9.4),(7.4,10)])
boolean(finger,cyl('H3 insert pocket',(0,0,8.5),2.2,3.0))
boolean(finger,cyl('Screw tip clearance',(0,0,6.7),1.7,4.4))
reports['04_Key_finger_button'] = finish(finger,'04_Key_finger_button',black)

# Slip cap seats on tip. Crown19 above box; bottom12. Not glued to shaft/lever.
cap = lathe('Shaft contact cap',[(5.1,0),(5.4,.3),(5.4,5),(5.2,5.5),
                              (4.7,6),(3.5,6.5),(1.5,6.9),(0,7)])
boolean(cap,cyl('Loose 6.3mm socket',(0,0,2),3.15,6))  # -1..5
reports['05_Key_shaft_cap_6p3mm'] = finish(cap,'05_Key_shaft_cap_6p3mm',black)
assert abs(span(cap,(0,0,2),(1,0,0))-6.3)<.01

params = dict(shaft_diameter_mm=6, shaft_type='User confirms same ribbed EC11 as front',
    nominal_shaft_tip_above_box_mm=17, top_projection_independently_measured=False,
    projection_basis='Same encoder and nominal3mm wall;17mm carried over from front',
    standalone=dict(diameter_mm=26,height_mm=19,socket_diameter_mm=6,socket_depth_mm=16,
        nominal_box_gap_mm=1,nut_recess_diameter_mm=13,nut_recess_depth_mm=5.5),
    key=dict(base_mm=[84,22,25.9], pivot_height_mm=19, cap_top_mm=19,
        pivot_to_shaft_mm=30,pivot_to_stop_mm=43,pivot_to_finger_mm=64,
        stop_post_top_mm=17,lever_underside_mm=19,cap_socket_diameter_mm=6.3,
        cap_socket_depth_mm=5,cap_height_mm=7,hinge_side_gap_mm=.3),
    hardware=['1 M3x12 pivot screw','2 M3x8 screws: stop and finger button',
              '3 M3xH3 heat-set inserts: hinge, stop, finger button'],
    insert_pilot_diameter_mm=4.4, insert_pilot_depth_mm=3,
    stop='Adjust to actual click, not an assumed travel; encoder must freely return',
    hinge_lift_stop='Integral bridge, underside24.35, nominal0.35mm gap above lever',
    physical_fit_verified=False, print='All supplied orientations, supports OFF',
    firmware='Same click/hold/triple-click actions; no manual Morse keying added')
(OUT/'geometry_check.json').write_text(json.dumps(reports,indent=2)+'\n')
(OUT/'design_parameters.json').write_text(json.dumps(params,indent=2)+'\n')

# Printable five-object file arranged on a small plate.
button.location=(20,-33,0); base.location=(-26,12,0); lever.location=(-26,35,0)
finger.location=(-17,-33,0); cap.location=(-42,-33,0)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Titanic_top_button_and_key_parts.blend'))

# Assembled exact print geometry, references are deliberately not exported.
button.location=(-41,-35,14.5); button.rotation_euler.x=math.pi
base.location=(-30,0,0); lever.location=(-30,0,24); lever.rotation_euler.x=math.pi
finger.location=(34,0,34); finger.rotation_euler.x=math.pi
cap.location=(0,0,12)
shaft=cyl('6mm EC11 reference',(0,0,9.7),3,14.6);shaft.data.materials.append(steel)
nut=cyl('Encoder mounting nut reference',(0,0,2.4),5.6,2.8,n=6);nut.data.materials.append(steel)
surface=box('Top of enclosure - illustrative mounting surface',(0,0,-2),(110,37,4),1);surface.data.materials.append(wood)
# Real screw reference envelopes, rest position gives0.8mm stop gap.
axle=cyl('M3x12 pivot shank',(-30,.7,19),1.5,12,'Y');axle.data.materials.append(steel)
head=cyl('Pivot head',(-30,-6.8,19),2.75,3,'Y');head.data.materials.append(steel)
for x,tip in [(13,17.8),(34,20.2)]:
    screw=cyl('M3x8 shank reference',(x,0,tip+4),1.5,8);screw.data.materials.append(steel)
    hz=tip+9.5 if x==13 else tip-1.5
    head=cyl('M3 head reference',(x,0,hz),2.75,3);head.data.materials.append(steel)

# Independent rest/down motion intersection test, omitting intended cap contact.
def overlap_volume(a,b,transform=None):
    q=a.copy();q.data=a.data.copy();scene.collection.objects.link(q)
    if transform is not None:q.matrix_world=transform @ q.matrix_world
    m=q.modifiers.new('Motion intersection','BOOLEAN');m.solver='EXACT';m.operation='INTERSECT';m.object=b
    bpy.context.view_layer.objects.active=q;bpy.ops.object.modifier_apply(modifier=m.name)
    bm=bmesh.new();bm.from_mesh(q.data);v=abs(bm.calc_volume(signed=True));bm.free()
    bpy.data.objects.remove(q,do_unlink=True);return v
bpy.context.view_layer.update();motion=[]
for shaft_down in [0,.3,.6,.85]:
    angle=math.asin(shaft_down/30)
    trans=Matrix.Translation((-30,0,19)) @ Matrix.Rotation(angle,4,'Y') @ Matrix.Translation((30,0,-19))
    v=overlap_volume(lever,base,trans);assert v<.001,(shaft_down,v)
    motion.append(dict(shaft_drop_mm=shaft_down,lever_base_overlap_mm3=v,
                       stop_drop_mm=43*shaft_down/30,finger_drop_mm=64*shaft_down/30))
# The up-stop must allow free rest/return but obstruct3mm upward cap motion,
# before the5mm-deep cap could lift clear of its shaft.
angle=math.asin(-3/30)
trans=Matrix.Translation((-30,0,19)) @ Matrix.Rotation(angle,4,'Y') @ Matrix.Translation((30,0,-19))
lift_collision=overlap_volume(lever,base,trans);assert lift_collision>.1,lift_collision
(OUT/'motion_check.json').write_text(json.dumps(dict(
    meaning='Printed lever/base clearance only, not measured click stroke or force',
    samples=motion,lift_stop_overlap_at_3mm_up_mm3=lift_collision,
    physical_click_return_tested=False),indent=2)+'\n')

def aim(o,p):o.rotation_euler=(Vector(p)-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(80,-150,115));cam=bpy.context.object
cam.data.type='ORTHO';cam.data.ortho_scale=135;scene.camera=cam;aim(cam,(0,-10,10))
floor=box('Studio floor',(0,0,-5),(800,800,1));floor.data.materials.append(mat('Warm neutral',(.28,.27,.25),0,.8))
for pos,energy,size in [((-60,-80,140),330000,90),((70,35,100),240000,75),((-70,60,65),160000,60)]:
    bpy.ops.object.light_add(type='AREA',location=pos);o=bpy.context.object;o.data.energy=energy;o.data.size=size;aim(o,(0,0,10))
scene.world.color=(.2,.2,.2);scene.render.engine='CYCLES';scene.cycles.samples=40;scene.cycles.use_denoising=True
scene.render.resolution_x=1500;scene.render.resolution_y=1050;scene.render.resolution_percentage=100
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Titanic_top_button_and_key_assembled.blend'))
scene.render.filepath=str(OUT/'Top_button_and_key_preview.png');bpy.ops.render.render(write_still=True)
print('PASS: five closed print solids, socket checks and lever/base motion clearance. Physical fit/return untested.',flush=True)
