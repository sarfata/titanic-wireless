"""Face-up two-colour front for the user's existing v0.8 bottom; dimensions in mm.
Blender --background --factory-startup --python blender/build_front_v11.py
No bottom revision and no unmeasured encoder knob is exported as a print part.
"""
import bpy,bmesh,json,math,struct,sys
from pathlib import Path
from mathutils import Vector,Matrix
from mathutils.bvhtree import BVHTree

ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'outputs/front_v11';OUT.mkdir(parents=True,exist_ok=True)
PARAMS={'body_mm':[200,140],'base_mm':3,'decoration_mm':.8,'corner_radius_mm':3,
 'screws_x_y_mm':[[-92,-62],[-92,62],[92,-62],[92,62]],'screw_holes_mm':3.4,
 'epaper_centre_mm':[-37,3],'epaper_window_mm':[85.4,64.2],
 'meter_centre_mm':[54,29],'meter_hole_mm':28,
 'camera_centre_mm':[54,8],'camera_hole_mm':10,
 'camera_basis':'M5Stack DXF lens is on case centreline, 21 mm below case centre; user confirms camera at bottom',
 'encoder_centre_mm':[52,-36],'encoder_hole_mm':8,'encoder_mount_thickness_mm':3,
 'encoder_basis':'User redrilled same EC11 top mount with 5/16 inch (7.9375 mm) bit',
 'spacers':{'count':0,'basis':'User requests flush seating on the rim; no spacers'},
 'screw_shrouds':{'outside_mm':10.4,'head_pocket_mm':6.8,'height_above_base_mm':3.2,'bearing_plane_mm':3,'outer_round_mm':.55,
 'fit_status':'Assumed head envelope up to diameter6.4/height3.0 mm; actual screw measurements pending'},
 'assembled_body_depth_mm':40,'depth_including_raised_details_mm':43.2,
 'bottom':'Existing printed v0.8, unchanged','lid_screws':'M3 x 8; 5 mm projection without spacers, 1 mm nominal bore-tip clearance',
 'fit_limits':'Front fit trial: visible-paper alignment, Core padding, camera field of view and encoder nut engagement need physical confirmation'}
(OUT/'design_parameters.json').write_text(json.dumps(PARAMS,indent=2)+'\n')
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=.001
scene.unit_settings.length_unit='MILLIMETERS'
col=bpy.data.collections.new('TITANIC_GENERATED');scene.collection.children.link(col)
def own(o,name):
 o.name=name
 for c in list(o.users_collection):c.objects.unlink(o)
 col.objects.link(o);return o
def material(name,colour,roughness=.5,metallic=0):
 m=bpy.data.materials.new(name);m.use_nodes=True;b=m.node_tree.nodes.get('Principled BSDF')
 b.inputs['Base Color'].default_value=(*colour,1);b.inputs['Roughness'].default_value=roughness;b.inputs['Metallic'].default_value=metallic
 return m
black=material('Charcoal PLA',(.028,.036,.038),.48)
gold=material('Brass-colour PLA',(.48,.29,.085),.4,.25)
steel=material('Steel hardware',(.32,.34,.36),.28,.8)
paper=material('Warm ePaper',(.8,.79,.7),.8)
def box(name,loc,size,mat=black):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=own(bpy.context.object,name);o.dimensions=size
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(mat);return o
def cylinder(name,loc,r,depth,mat=black,vertices=128):
 bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=r,depth=depth,location=loc)
 o=own(bpy.context.object,name);o.data.materials.append(mat);return o
def boolean(a,b,op='DIFFERENCE'):
 m=a.modifiers.new(op+' '+b.name,'BOOLEAN');m.solver='MANIFOLD';m.operation=op;m.object=b
 bpy.context.view_layer.objects.active=a;bpy.ops.object.modifier_apply(modifier=m.name)
 bpy.data.objects.remove(b,do_unlink=True)
def bevel(o,width,segments=5,vertical=False):
 if vertical:
  attr=o.data.attributes.new('bevel_weight_edge','FLOAT','EDGE')
  for e in o.data.edges:
   a,b=[o.data.vertices[v].co for v in e.vertices]
   attr.data[e.index].value=float(abs(a.z-b.z)>.01 and abs(a.x-b.x)<.001 and abs(a.y-b.y)<.001)
 m=o.modifiers.new('Soft edges','BEVEL');m.width=width;m.segments=segments
 m.limit_method='WEIGHT' if vertical else 'ANGLE';m.angle_limit=.5
 bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=m.name)
def mesh_of(o):
 bpy.context.view_layer.update();deps=bpy.context.evaluated_depsgraph_get()
 mesh=bpy.data.meshes.new_from_object(o.evaluated_get(deps),depsgraph=deps);mesh.transform(o.matrix_world);return mesh
def new_mesh(name,mesh):
 o=bpy.data.objects.new(name,mesh);col.objects.link(o);return o
def audit(mesh):
 bm=bmesh.new();bm.from_mesh(mesh);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.00001)
 bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
 remaining=set(bm.verts);components=0
 while remaining:
  components+=1;stack=[remaining.pop()]
  while stack:
   for edge in stack.pop().link_edges:
    for v in edge.verts:
     if v in remaining:remaining.remove(v);stack.append(v)
 r={'nonmanifold_edges':sum(not e.is_manifold for e in bm.edges),'degenerate_faces':sum(f.calc_area()<1e-9 for f in bm.faces),
    'connected_components':components,'volume_mm3':round(bm.calc_volume(signed=True),3)}
 bm.to_mesh(mesh);bm.free();mesh.update();assert r['nonmanifold_edges']==0 and r['degenerate_faces']==0 and r['volume_mm3']>0,r
 return r
def export_stl(o,name):
 mesh=mesh_of(o);r=audit(mesh);assert r['connected_components']==1,r
 mesh.calc_loop_triangles();folder=OUT/'stl';folder.mkdir(exist_ok=True)
 with (folder/name).open('wb') as f:
  f.write(b'Titanic front v1.1, mm, face up'.ljust(80,b'\0'));f.write(struct.pack('<I',len(mesh.loop_triangles)))
  for tri in mesh.loop_triangles:
   p=[mesh.vertices[v].co for v in tri.vertices];n=(p[1]-p[0]).cross(p[2]-p[0]).normalized()
   f.write(struct.pack('<12fH',*n,*p[0],*p[1],*p[2],0))
 r['file']=name;r['xyz_mm']=[round(max(v.co[i] for v in mesh.vertices)-min(v.co[i] for v in mesh.vertices),4) for i in range(3)]
 return r

base=box('Front black base',(0,0,1.5),(200,140,3));bevel(base,3,8,True);bevel(base,.35,4)
boolean(base,box('Paper opening',(-37,3,1.5),(85.4,64.2,8)))
for name,x,y,r in [('LCD',54,29,14),('Camera',54,8,5),('EC11',52,-36,4)]+[('M3',x,y,1.7) for x,y in PARAMS['screws_x_y_mm']]:
 boolean(base,cylinder(name+' hole',(x,y,1.5),r,8))
decor=[]
def plaque(name,centre,size):
 o=box(name,(*centre,3.4),(*size,.8),gold);decor.append(o);return o
surround=plaque('Paper surround',(-37,3),(95.4,74.2));bevel(surround,1.2,6,True)
boolean(surround,box('Paper aperture',(-37,3,3.4),(85.4,64.2,4)))
ring=cylinder('Meter and camera brass surround',(54,29,3.4),20,.8,gold)
boolean(ring,cylinder('Camera surround',(54,8,3.4),7,.8,gold),'UNION')
boolean(ring,cylinder('Meter aperture',(54,29,3.4),14,4));boolean(ring,cylinder('Camera aperture',(54,8,3.4),5,4));decor.append(ring)
dial=cylinder('Tuning dial surround',(52,-36,3.4),26.5,.8,gold);boolean(dial,cylinder('Clear encoder nut and knob',(52,-36,3.4),21,4));decor.append(dial)
for degrees in range(0,360,30):
 angle=math.radians(degrees);r=25.0
 tick=box('Dial engraved tick',(52+math.sin(angle)*r,-36+math.cos(angle)*r,3.4),(.65,2,.8+1))
 tick.rotation_euler.z=-angle;boolean(dial,tick)
for name,body,y,width,size in [('Vessel','R.M.S. TITANIC',55,99,7),('Office','WIRELESS OFFICE',-52,101,5.1)]:
 p=plaque(name+' plaque',(-37,y),(width,13));bevel(p,.8,5,True)
 bpy.ops.object.text_add(location=(-37,y,2.9));t=own(bpy.context.object,name+' engraving')
 t.data.body=body;t.data.align_x='CENTER';t.data.align_y='CENTER';t.data.size=size;t.data.extrude=1.2;t.data.resolution_u=10
 bpy.ops.object.convert(target='MESH');t=bpy.context.object
 bm=bmesh.new();bm.from_mesh(t.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.0001)
 bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(t.data);bm.free();boolean(p,t)

# Rounded open wells hide screw-head sides while keeping the driver accessible.
# Bearing surface remains at 3 mm, so the user's M3x8 screws do not project farther.
for x,y in PARAMS['screws_x_y_mm']:
 shroud=cylinder('Rounded screw surround '+str((x,y)),(x,y,4.6),5.2,3.2,gold)
 bevel(shroud,.55,6)
 boolean(shroud,cylinder('Head and driver access',(x,y,4.6),3.4,8))
 decor.append(shroud)

# The slicer's material volumes meet at exactly 3 mm, producing one colour change.
colour=[]
for o,extruder in [(base,1)]+[(d,2) for d in decor]:
 mesh=mesh_of(o);audit(mesh);mesh.transform(Matrix.Translation((100,70,0)));mesh.calc_loop_triangles()
 colour.append({'name':o.name,'extruder':extruder,'vertices':[list(v.co) for v in mesh.vertices],'triangles':[list(t.vertices) for t in mesh.loop_triangles]})
(OUT/'front_colour_meshes.json').write_text(json.dumps(colour))
# Union an overlapping copy for a single-solid STL and the render; material parts
# above remain exactly adjacent in the 3MF, without coincident overlapping volumes.
front=new_mesh('Titanic front v1.1',mesh_of(base));front.data.materials.clear();front.data.materials.append(black);front.data.materials.append(gold)
for d in decor:
 mesh=mesh_of(d)
 for v in mesh.vertices:
  if abs(v.co.z-3)<.0001:v.co.z=2.98
 mesh.materials.clear();mesh.materials.append(black);mesh.materials.append(gold)
 for p in mesh.polygons:p.material_index=1
 boolean(front,new_mesh('Colour union',mesh),'UNION');d.hide_render=True;d.hide_set(True)
base.hide_render=True;base.hide_set(True)
front['print_file']='01_Titanic_front_v11';front['print_orientation']='Z_UP'
front['print_status']='Fits existing v0.8 bottom datums; first full front fit trial'
reports=[export_stl(front,'01_Titanic_front_v11.stl')]
(OUT/'print_mesh_check.json').write_text(json.dumps(reports,indent=2)+'\n')

# Measure apertures and bolt locations on the evaluated mesh, not just parameters.
mesh=mesh_of(front);bm=bmesh.new();bm.from_mesh(mesh);tree=BVHTree.FromBMesh(bm);bm.free();checks=[]
def span(name,x,y,z,axis,wanted):
 origin=Vector((x,y,z));d=Vector((0,0,0));d[axis]=1
 a=tree.ray_cast(origin,d,500)[0];b=tree.ray_cast(origin,-d,500)[0]
 actual=a[axis]-b[axis];ok=abs(actual-wanted)<.01
 checks.append({'feature':name,'actual_mm':round(actual,4),'expected_mm':wanted,'pass':ok});assert ok,checks[-1]
span('Paper width',-37,3,1,0,85.4);span('Paper height',-37,3,1,1,64.2)
span('Round meter',54,29,1,0,28);span('Camera lens',54,8,1,0,10);span('EC11',52,-36,1,0,8)
for x,y in PARAMS['screws_x_y_mm']:span(f'M3 {x}/{y}',x,y,1,0,3.4)
for x,y in [(54,29),(54,8),(52,-36),(-37,3)]+PARAMS['screws_x_y_mm']:
 assert tree.ray_cast(Vector((x,y,10)),Vector((0,0,-1)),20)[0] is None
checks.append({'feature':'All apertures pass completely through panel and decoration','pass':True})
# Black only around the encoder nut: actual mount thickness must stay 3 mm.
p=tree.ray_cast(Vector((59,-36,10)),Vector((0,0,-1)),20)[0];assert abs(p.z-3)<.001
checks.append({'feature':'EC11 nut seat remains 3 mm thick','actual_mm':p.z,'pass':True})
for x,y in PARAMS['screws_x_y_mm']:
 span(f'Head recess {x}/{y}',x,y,4.2,0,6.8)
 p=tree.ray_cast(Vector((x+2.3,y,10)),Vector((0,0,-1)),20)[0]
 assert abs(p.z-3)<.001
 checks.append({'feature':f'M3 bearing plane {x}/{y}','actual_mm':p.z,'pass':True})
 checks.append({'feature':f'Head diameter6.4 has0.2 mm radial clearance {x}/{y}','pass':True})
(OUT/'feature_check.json').write_text(json.dumps(checks,indent=2)+'\n')

# Studio lighting, actual part face-up. Neither ground nor hardware is exported.
groundmat=material('Studio warm grey',(.19,.18,.16),.85)
bpy.ops.mesh.primitive_plane_add(size=2000,location=(0,0,-.15));ground=bpy.context.object;ground.name='Studio ground';ground.data.materials.append(groundmat)
bpy.ops.object.camera_add(location=(155,-250,330));cam=bpy.context.object;cam.data.type='ORTHO';cam.data.ortho_scale=265;cam.data.clip_end=5000;scene.camera=cam
def aim(o,target):o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
aim(cam,(0,0,0))
for loc,power,size in [((-150,-200,330),1700000,200),((240,80,240),1200000,180),((-170,160,180),650000,140)]:
 bpy.ops.object.light_add(type='AREA',location=loc);l=bpy.context.object;l.data.energy=power;l.data.size=size;aim(l,(0,0,20))
scene.world.color=(.22,.22,.22);scene.render.engine='CYCLES';scene.cycles.samples=48;scene.cycles.use_denoising=True
scene.render.resolution_x=1600;scene.render.resolution_y=1200;scene.render.resolution_percentage=100
scene.render.filepath=str(OUT/'Titanic_front_v11_print.png')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Titanic_front_v11.blend'));bpy.ops.render.render(write_still=True)

# Assembled view uses the exact v0.8 bottom STL and the newly exported front.
assembled=[];front.location.z=37;assembled.append(front)
bpy.ops.wm.stl_import(filepath=str(ROOT/'outputs/bottom_v08/stl/01_Titanic_bottom_v08.stl'))
bottom=bpy.context.object;bottom.name='Existing v0.8 bottom (unchanged)';bottom.location=(-111,-70,0);bottom.data.materials.append(black);assembled.append(bottom)
for x,y in PARAMS['screws_x_y_mm']:
 s=cylinder('M3 x 8 screw head',(x,y,41.5),2.75,3,steel,64)
 boolean(s,cylinder('Hex recess',(x,y,42.9),1.3,1,black,6));assembled.append(s)
def textured(name,path,loc,w,h):
 m=material(name,(.8,.8,.75));nodes=m.node_tree.nodes;b=nodes.get('Principled BSDF');t=nodes.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(path));t.image.pack()
 m.node_tree.links.new(t.outputs['Color'],b.inputs['Base Color']);m.node_tree.links.new(t.outputs['Color'],b.inputs['Emission Color']);b.inputs['Emission Strength'].default_value=.1
 bpy.ops.mesh.primitive_plane_add(size=2,location=loc);o=own(bpy.context.object,name);o.scale=(w/2,h/2,1);o.data.materials.append(m);assembled.append(o)
textured('Certificate on actual ePaper opening',ROOT/'outputs/epaper_certificate_v06.png',(-37,3,37),85.4,64.2)
textured('Round S-meter artwork',ROOT/'outputs/meter_keyed_5.6.png',(54,29,36.8),28.2,28.2)
assembled.append(cylinder('Camera lens (reference)',(54,8,36.6),2,1,black))
knob=cylinder('Illustrative EC11 tuning knob - not a fitted print part',(52,-36,46.5),19.5,10,black);bevel(knob,1,5);assembled.append(knob)
for i in range(48):
 a=2*math.pi*i/48;o=cylinder('Knob grip reference',(52+19*math.cos(a),-36+19*math.sin(a),46.5),.7,8,black,12);assembled.append(o)
o=box('Knob pointer',(52,-20,51.6),(1,4,.3),gold);assembled.append(o)
for name,y,r,depth,mat in [('Top EC11 collar',71.2,5,2.4,steel),('Top push cap illustration',77.5,10,9,black)]:
 o=cylinder(name,(-41,y,18.5),r,depth,mat);o.rotation_euler.x=-math.pi/2;assembled.append(o)
R=Matrix.Translation((0,0,70))@Matrix.Rotation(math.pi/2,4,'X')
bpy.context.view_layer.update()
for o in assembled:o.matrix_world=R@o.matrix_world
cam.location=(220,-340,225);cam.data.ortho_scale=300;aim(cam,(0,-17,73));scene.cycles.samples=72
scene.render.filepath=str(OUT/'Titanic_front_v11_assembled.png')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Titanic_front_v11_assembled.blend'));bpy.ops.render.render(write_still=True)
print('PASS: front mesh, measured apertures; exported black/gold volumes and assembled view',flush=True)
