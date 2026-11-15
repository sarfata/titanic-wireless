"""Original nautical costume badges in mm: two shoulder boards and one anchor medallion.
Flat backs on bed; one gold filament; no supports. Decorative, not historical rank replicas.
"""
import bpy, bmesh, math, json, struct
from pathlib import Path
from mathutils import Vector

OUT=Path(__file__).resolve().parents[1]/'outputs/costume_badges_v01'
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
scene=bpy.context.scene
scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=.001
scene.unit_settings.length_unit='MILLIMETERS'
def mat(name,color,rough=.38,metal=.7):
    m=bpy.data.materials.new(name);m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value=(*color,1)
    p.inputs['Roughness'].default_value=rough;p.inputs['Metallic'].default_value=metal
    return m
gold=mat('Gold PLA appearance',(.67,.39,.09),.34,.65)
def poly(name,points,z0,z1):
    n=len(points);v=[(x,y,z) for z in [z0,z1] for x,y in points]
    faces=[tuple(reversed(range(n))),tuple(range(n,2*n))]
    faces += [(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    me=bpy.data.meshes.new(name);me.from_pydata(v,[],faces);me.update()
    o=bpy.data.objects.new(name,me);scene.collection.objects.link(o);return o
def rounded(name,w,h,r,z0,z1,x=0,y=0):
    pts=[]
    for cx,cy,start in [(w/2-r,h/2-r,0),(-w/2+r,h/2-r,90),(-w/2+r,-h/2+r,180),(w/2-r,-h/2+r,270)]:
        for k in range(13):
            a=math.radians(start+k*90/12);pts.append((x+cx+r*math.cos(a),y+cy+r*math.sin(a)))
    return poly(name,pts,z0,z1)
def ellipse(name,rx,ry,z0,z1,x=0,y=0):
    return poly(name,[(x+rx*math.cos(i*math.tau/192),y+ry*math.sin(i*math.tau/192)) for i in range(192)],z0,z1)
def boolean(o,tool,operation='UNION'):
    bpy.context.view_layer.objects.active=o
    m=o.modifiers.new(operation,'BOOLEAN');m.operation=operation;m.solver='EXACT';m.object=tool
    bpy.ops.object.modifier_apply(modifier=m.name);bpy.data.objects.remove(tool,do_unlink=True)
def bevel(o,width=.28):
    bm=bmesh.new();bm.from_mesh(o.data)
    bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.00001)
    bmesh.ops.dissolve_limit(bm,angle_limit=.0001,verts=list(bm.verts),edges=list(bm.edges))
    bm.to_mesh(o.data);bm.free();o.data.update()
    bpy.context.view_layer.objects.active=o
    m=o.modifiers.new('Soft edges','BEVEL');m.width=width;m.segments=3
    bpy.ops.object.modifier_apply(modifier=m.name)
def anchor(target,s,cy,z0,z1):
    def add(p):boolean(target,p)
    ring=ellipse('Anchor eye',4.6*s,4.6*s,z0,z1,0,cy+17*s)
    boolean(ring,ellipse('Eye opening',2.4*s,2.4*s,z0-1,z1+1,0,cy+17*s),'DIFFERENCE')
    add(ring)
    add(rounded('Anchor shank',4.2*s,33*s,1*s,z0,z1,0,cy-1.5*s))
    add(rounded('Anchor stock',28*s,4.2*s,1.5*s,z0,z1,0,cy+8*s))
    # Upturned arms and rounded arrow-shaped flukes, all connected to the shank.
    p=[(0,-21),(8,-19),(15,-14),(19,-7),(20,-3),(12,-5),(14,-8),(11,-12),(6,-15),(0,-17),
       (-6,-15),(-11,-12),(-14,-8),(-12,-5),(-20,-3),(-19,-7),(-15,-14),(-8,-19)]
    add(poly('Anchor arms',[(x*s,cy+y*s) for x,y in p],z0,z1))
def shoulder():
    o=rounded('Shoulder board',35,80,5,0,1.6)
    bevel(o,.25)
    rim=rounded('Shoulder border',33,78,4,1.35,2.3)
    boolean(rim,rounded('Border inside',30.6,75.6,2.8,.9,3),'DIFFERENCE');boolean(o,rim)
    for y in [-29,-18,-7,4]:
        stripe=rounded('Raised stripe',28,6.4,1,1.35,3.2,0,y)
        bevel(stripe,.25);boolean(o,stripe)
    anchor(o,.52,24,1.35,3.2)
    o.data.materials.clear();o.data.materials.append(gold)
    for f in o.data.polygons:f.material_index=0
    return o
def chest():
    o=ellipse('Anchor chest medallion',27,31,0,1.6)
    bevel(o,.25)
    rim=ellipse('Medallion rim',25.8,29.8,1.35,2.4)
    boolean(rim,ellipse('Medallion rim inside',24.2,28.2,1,3),'DIFFERENCE');boolean(o,rim)
    anchor(o,1.05,1,1.35,3.2)
    # Four tiny raised rivets echo the brass details on the radio.
    for x,y in [(-21,0),(21,0),(0,26),(0,-26)]:
        boolean(o,ellipse('Rivet',1.25,1.25,1.35,2.8,x,y))
    o.data.materials.clear();o.data.materials.append(gold)
    for f in o.data.polygons:f.material_index=0
    return o
def audit_export(o,name,dimensions):
    bm=bmesh.new();bm.from_mesh(o.data)
    bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.00001)
    bmesh.ops.dissolve_degenerate(bm,dist=.00001,edges=list(bm.edges))
    bmesh.ops.dissolve_limit(bm,angle_limit=.0001,verts=list(bm.verts),edges=list(bm.edges))
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    todo=set(bm.verts);components=0
    while todo:
        components+=1;stack=[todo.pop()]
        while stack:
            for e in stack.pop().link_edges:
                for v in e.verts:
                    if v in todo:todo.remove(v);stack.append(v)
    r={'nonmanifold_edges':sum(not e.is_manifold for e in bm.edges),
       'degenerate_faces':sum(f.calc_area()<1e-9 for f in bm.faces),'components':components,
       'volume_mm3':bm.calc_volume(signed=True),'physical_fit_verified':False}
    assert r['nonmanifold_edges']==0 and r['degenerate_faces']==0 and components==1 and r['volume_mm3']>0,r
    # Back stays planar at z0; no inaccessible pockets or printed fasteners.
    bounds=[[min(v.co[i] for v in bm.verts),max(v.co[i] for v in bm.verts)] for i in range(3)]
    r['dimensions_mm']=[b-a for a,b in bounds];r['flat_back_z_mm']=bounds[2][0]
    assert abs(bounds[2][0])<.001
    for got,want in zip(r['dimensions_mm'],dimensions):assert abs(got-want)<.03,(got,want)
    bm.to_mesh(o.data);bm.free();o.data.update();o.data.calc_loop_triangles()
    with (OUT/name).open('wb') as f:
        f.write(b'Original nautical costume badge; mm; flat back on bed'.ljust(80,b'\0'))
        f.write(struct.pack('<I',len(o.data.loop_triangles)))
        for t in o.data.loop_triangles:
            p=[o.data.vertices[i].co for i in t.vertices]
            normal=(p[1]-p[0]).cross(p[2]-p[0]).normalized()
            f.write(struct.pack('<12fH',*normal,*p[0],*p[1],*p[2],0))
    r['stl']=name;return r

left=shoulder();left.name='01 Shoulder left - 80 x 35 mm'
report=[audit_export(left,'01_Shoulder_left_80x35.stl',[35,80,3.2])]
right=left.copy();right.data=left.data.copy();scene.collection.objects.link(right);right.name='02 Shoulder right - 80 x 35 mm'
report.append(audit_export(right,'02_Shoulder_right_80x35.stl',[35,80,3.2]))
badge=chest();badge.name='03 Anchor chest badge - 54 x 62 mm'
report.append(audit_export(badge,'03_Anchor_chest_54x62.stl',[54,62,3.2]))
left.location=(-53,0,0);right.location=(53,0,0);badge.location=(0,-8,0)
parameters={'units':'mm','pieces':3,'shoulder_mm':[80,35,3.2],'chest_mm':[54,62,3.2],
 'base_thickness_mm':1.6,'layer_height_mm':.2,'supports':False,'material':'single gold PLA',
 'orientation':'flat back on bed, decorations up','attachment':'Adhesive clip or safety bar pin on flat back; hardware not included',
 'historical_status':'Original nautical costume design; not an authenticated Titanic or naval rank badge',
 'dimensions_status':'Compact defaults; jacket measurements not yet provided',
 'clear_adhesive_area':'Each shoulder has a central 28 x 65 mm flat back; chest has central 38 x 30 mm flat back',
 'physical_fit_verified':False}
(OUT/'geometry_check.json').write_text(json.dumps(report,indent=2)+'\n')
(OUT/'design_parameters.json').write_text(json.dumps(parameters,indent=2)+'\n')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Titanic_costume_badges_v01.blend'))

# Render the printable geometry on navy cloth. No unprintable visual decorations.
navy=mat('Navy wool',(.008,.018,.037),.92,0)
nodes=navy.node_tree.nodes;links=navy.node_tree.links
noise=nodes.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=550
bump=nodes.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.22;bump.inputs['Distance'].default_value=.12
links.new(noise.outputs['Fac'],bump.inputs['Height']);links.new(bump.outputs['Normal'],nodes.get('Principled BSDF').inputs['Normal'])
bpy.ops.mesh.primitive_plane_add(size=900,location=(0,0,-.05));bpy.context.object.data.materials.append(navy)
def aim(o,p):o.rotation_euler=(Vector(p)-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(35,-125,225));cam=bpy.context.object;cam.data.type='ORTHO';cam.data.ortho_scale=179;scene.camera=cam;aim(cam,(0,0,0))
for pos,energy,size in [((-80,-90,160),450000,95),((110,45,125),330000,85),((-90,80,90),260000,65)]:
    bpy.ops.object.light_add(type='AREA',location=pos);o=bpy.context.object;o.data.energy=energy;o.data.size=size;aim(o,(0,0,0))
scene.world.color=(.22,.22,.22);scene.render.engine='CYCLES';scene.cycles.samples=48;scene.cycles.use_denoising=True
scene.render.resolution_x=1500;scene.render.resolution_y=1100;scene.render.resolution_percentage=100
scene.render.filepath=str(OUT/'Gold_costume_badges_preview.png');bpy.ops.render.render(write_still=True)
cam.location=(0,0,220);aim(cam,(0,0,0));cam.data.ortho_scale=172
scene.render.filepath=str(OUT/'Gold_costume_badges_top.png');bpy.ops.render.render(write_still=True)
print('PASS: three closed single solids, flat backs at Z0, no supports. '+str(OUT),flush=True)
