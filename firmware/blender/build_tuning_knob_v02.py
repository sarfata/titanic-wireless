"""Printable EC11 tuning knob, mm. User measured ribbed shaft: diameter6 mm, tip17 mm above front, ribbed length12 mm.
Print decorative face on bed, socket upwards. Push-on retention; no screw, insert or radial opening.
"""
import bpy,bmesh,json,math,struct
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree

ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'outputs/tuning_knob_v02';OUT.mkdir(parents=True,exist_ok=True)
P={'diameter_mm':39,'height_mm':19,'grip_flutes':40,'edge_round_mm':1,
   'measured_shaft_diameter_mm':6,'measured_shaft_tip_above_front_mm':17,'measured_ribbed_length_mm':12,
   'socket_diameter_mm':6.0,'socket_depth_mm':16,'installed_panel_gap_mm':1,'shaft_tip_clearance_mm':0,
   'lead_in_chamfer_mm':.6,'retention':'Push fit on ribbed shaft; no hardware',
   'nut_recess_diameter_mm':13,'nut_recess_depth_mm':5.5,
   'physical_fit_verified':False,'shaft_status':'User measured 6 mm ribbed shaft, tip17 mm above panel, ribbed length12 mm; printed fit untested',
   'print_orientation':'Decorative face down at Z0, shaft socket open upwards',
   'assembly':'Push on gently; 16 mm socket depth leaves nominal1 mm rear/panel gap at the shaft tip stop. Do not force a tight print.'}
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=.001;scene.unit_settings.length_unit='MILLIMETERS'
def material(name,color,rough=.5,metal=0):
 m=bpy.data.materials.new(name);m.use_nodes=True;b=m.node_tree.nodes.get('Principled BSDF')
 b.inputs['Base Color'].default_value=(*color,1);b.inputs['Roughness'].default_value=rough;b.inputs['Metallic'].default_value=metal;return m
black=material('Black PLA',(.035,.043,.046),.42)
def mesh(name,verts,faces):
 data=bpy.data.meshes.new(name);data.from_pydata(verts,[],faces);data.update()
 o=bpy.data.objects.new(name,data);scene.collection.objects.link(o);return o
def cut(o,cutter):
 mod=o.modifiers.new('Cut '+cutter.name,'BOOLEAN');mod.solver='MANIFOLD';mod.operation='DIFFERENCE';mod.object=cutter
 bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name);bpy.data.objects.remove(cutter,do_unlink=True)
def cylinder(name,r,z0,z1):
 bpy.ops.mesh.primitive_cylinder_add(vertices=128,radius=r,depth=z1-z0,location=(0,0,(z0+z1)/2));o=bpy.context.object;o.name=name;return o
# Continuous rounded flutes; all rings share vertices and produce one closed solid.
n=640;profiles=[]
for j in range(7):
 a=j*math.pi/12;profiles.append((1-math.cos(a),math.sin(a)))
profiles.append((18,1))
for j in range(1,7):
 a=j*math.pi/12;profiles.append((18+math.sin(a),math.cos(a)))
v=[]
for z,round_offset in profiles:
 for i in range(n):
  a=2*math.pi*i/n;r=18.1+round_offset+.4*math.cos(40*a);v.append((r*math.cos(a),r*math.sin(a),z))
faces=[tuple(reversed(range(n))),tuple(range((len(profiles)-1)*n,len(profiles)*n))]
for j in range(len(profiles)-1):
 for i in range(n):
  k=j*n+i;next=j*n+(i+1)%n;faces.append((k,next,next+n,k+n))
knob=mesh('Titanic tuning knob - measured 6 mm ribbed shaft',v,faces);knob.data.materials.append(black)
cut(knob,cylinder('Blind push-fit shaft socket',3,3,20))
cut(knob,cylinder('Encoder nut relief',6.5,13.5,20))
# Entry chamfer in the socket at the nut-relief shoulder.
bpy.ops.mesh.primitive_cone_add(vertices=128,radius1=3,radius2=3.6,depth=.6,location=(0,0,13.2))
cut(knob,bpy.context.object)
# Small recessed paint mark on the face; short 1.2 mm bridge in print orientation.
bpy.ops.mesh.primitive_cube_add(size=1,location=(0,14.5,.2));mark=bpy.context.object;mark.dimensions=(1.2,4,.8)
bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);cut(knob,mark)

bm=bmesh.new();bm.from_mesh(knob.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.00001)
bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
remaining=set(bm.verts);components=0
while remaining:
 components+=1;stack=[remaining.pop()]
 while stack:
  for e in stack.pop().link_edges:
   for vertex in e.verts:
    if vertex in remaining:remaining.remove(vertex);stack.append(vertex)
report={'nonmanifold_edges':sum(not e.is_manifold for e in bm.edges),'degenerate_faces':sum(f.calc_area()<1e-9 for f in bm.faces),
        'connected_components':components,'volume_mm3':bm.calc_volume(signed=True),'physical_fit_verified':False}
assert report['nonmanifold_edges']==0 and report['degenerate_faces']==0 and components==1 and report['volume_mm3']>0,report
tree=BVHTree.FromBMesh(bm)
def hit(p,d):return tree.ray_cast(Vector(p),Vector(d),100)[0]
def span(p,d):
 a=hit(p,d);b=hit(p,[-x for x in d]);assert a is not None and b is not None;return (a-b).length
checks={'socket_diameter_mm':span((0,0,7),(0,1,0)),
 'nut_relief_diameter_mm':span((0,0,14),(0,1,0)),
 'socket_floor_z_mm':hit((0,0,20),(0,0,-1)).z}
for key,want in zip(checks,[6,13,3]):assert abs(checks[key]-want)<.02,(key,checks[key],want)
# Verify the former radial hole is solid all around, at several heights.
for z in [6,9,12]:
 for i in range(40):
  a=2*math.pi*i/40;d=Vector((math.cos(a),math.sin(a),0))
  wall=hit(d*30+Vector((0,0,z)),-d)
  assert wall is not None and abs(Vector((wall.x,wall.y,0)).length-19.5)<.02
report['continuous_side_wall_verified']=True
# The16 mm socket seats at the17 mm shaft tip, giving1 mm nominal panel gap.
socket_depth=P['height_mm']-checks['socket_floor_z_mm']
report['installed_panel_gap_mm']=P['measured_shaft_tip_above_front_mm']-socket_depth
assert abs(report['installed_panel_gap_mm']-1)<.01
bm.to_mesh(knob.data);bm.free();knob.data.update();knob.data.calc_loop_triangles()
bounds=[[min(vertex.co[i] for vertex in knob.data.vertices),max(vertex.co[i] for vertex in knob.data.vertices)] for i in range(3)]
report['bounds_mm']=bounds;report['dimensions_mm']=[b-a for a,b in bounds];report['measured_features']=checks
for got,want in zip(report['dimensions_mm'],[39,39,19]):assert abs(got-want)<.01
with (OUT/'Titanic_tuning_knob_6mm_pushfit_v02.stl').open('wb') as f:
 f.write(b'Titanic tuning knob, mm, face down; measured 6 mm ribbed shaft'.ljust(80,b'\0'));f.write(struct.pack('<I',len(knob.data.loop_triangles)))
 for tri in knob.data.loop_triangles:
  p=[knob.data.vertices[i].co for i in tri.vertices];normal=(p[1]-p[0]).cross(p[2]-p[0]).normalized()
  f.write(struct.pack('<12fH',*normal,*p[0],*p[1],*p[2],0))
(OUT/'geometry_check.json').write_text(json.dumps(report,indent=2)+'\n')
(OUT/'design_parameters.json').write_text(json.dumps(P,indent=2)+'\n')

# Save the actual printable part alone at the origin before staging the preview.
knob['print_orientation']=P['print_orientation'];knob['fit_status']=P['shaft_status']
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Titanic_tuning_knob_v02.blend'))
back=knob.copy();back.data=knob.data.copy();scene.collection.objects.link(back);back.location.x=25
knob.location=(-25,0,19);knob.rotation_euler.x=math.pi
groundmat=material('Warm studio grey',(.28,.265,.235),.85)
bpy.ops.mesh.primitive_plane_add(size=500,location=(0,0,-.1));bpy.context.object.data.materials.append(groundmat)
def aim(o,p):o.rotation_euler=(Vector(p)-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(70,-125,125));cam=bpy.context.object;cam.data.type='ORTHO';cam.data.ortho_scale=110;scene.camera=cam;aim(cam,(0,0,6))
for p,energy,size in [((-50,-70,110),260000,80),((60,20,100),220000,70),((-60,60,55),130000,50)]:
 bpy.ops.object.light_add(type='AREA',location=p);o=bpy.context.object;o.data.energy=energy;o.data.size=size;aim(o,(0,0,5))
scene.world.color=(.2,.2,.2);scene.render.engine='CYCLES';scene.cycles.samples=32;scene.cycles.use_denoising=True
scene.render.resolution_x=1200;scene.render.resolution_y=700;scene.render.resolution_percentage=100
scene.render.filepath=str(OUT/'Titanic_tuning_knob_preview.png');bpy.ops.render.render(write_still=True)
print('PASS: one closed solid, measured push-fit socket, continuous sidewall and1 mm panel gap, 39 x 39 x19 mm; designed for user measurements, first printed fit pending',flush=True)
