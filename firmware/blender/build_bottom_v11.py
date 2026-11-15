"""Bottom v1.1: clear all four Core faces with low corner locators at Z9.
Keep the3.4 mm ePaper shift and NE USB relief; open the top EC11 bore to8 mm.
Print coordinates: X across face, Y toward radio top, Z from back toward front.
"""
import bpy,bmesh,json,math,struct
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'outputs/bottom_v11'
(OUT/'stl').mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'outputs/usb_access_v02/Titanic_USB_mount_proposal_v02.blend'))
body=next(o for o in bpy.data.objects if o.name.startswith('DRAFT shifted ePaper'))
old_label=next(o for o in bpy.data.objects if o.name=='Revision engraving')
corners=[old_label.matrix_world@Vector(p) for p in old_label.bound_box]
label_bounds=[(min(p[i] for p in corners),max(p[i] for p in corners)) for i in range(2)]
for o in list(bpy.data.objects):
    if o!=body and o.type not in {'LIGHT','CAMERA'} and o.name!='Studio ground':bpy.data.objects.remove(o,do_unlink=True)
body.name='Titanic bottom v1.1 - Core ports clear';body.hide_set(False);body.hide_render=False
body['print_file']='01_Titanic_bottom_v11';body['print_release']=True
body['status']='Printable revised bottom; first physical fit pending'
def cube(name,loc,dims):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.name=name;o.dimensions=dims
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);return o
def boolean(o,c,op='DIFFERENCE',remove=True):
    m=o.modifiers.new(op+' '+c.name,'BOOLEAN');m.solver='MANIFOLD';m.operation=op;m.object=c
    bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=m.name)
    if remove:bpy.data.objects.remove(c,do_unlink=True)
def closed_mesh(name,verts,faces):
    m=bpy.data.meshes.new(name);m.from_pydata(verts,[],faces);m.update()
    bm=bmesh.new();bm.from_mesh(m);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(m);bm.free()
    o=bpy.data.objects.new(name,m);bpy.context.scene.collection.objects.link(o);return o
# Through-hole on the RIGHT side when viewing the front. Big enough to feed
# the11.5 x8 housing through, rather than trapping a plug on a3.9 mm cable.
verts=[]
levels=[(95,.5),(97.6,.5),(98.1,0),(99.5,0),(100,.5),(103,.5)]
for x,flare in levels:
    hw,hh,r=6.5+flare,5+flare,1+flare
    for cy,cz,start in [(hw-r,hh-r,0),(-hw+r,hh-r,90),(-hw+r,-hh+r,180),(hw-r,-hh+r,270)]:
        for i in range(9):
            a=math.radians(start+i*90/8);verts.append((x,-35+cy+r*math.cos(a),18.5+cz+r*math.sin(a)))
n=36;faces=[tuple(reversed(range(n))),tuple(range((len(levels)-1)*n,len(verts)))]
for j in range(len(levels)-1):
    for i in range(n):faces.append((j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i))
boolean(body,closed_mesh('Side cable hole13 x10 R1, eased mouths',verts,faces))
# Apply the user's earlier5/16-inch drill feedback without changing its location.
bpy.ops.mesh.primitive_cylinder_add(vertices=128,radius=4,depth=8,location=(-41,68.5,18.5),rotation=(math.pi/2,0,0))
boolean(body,bpy.context.object)
# Open both connector sides above Z9. The original tall L guides intruded into
# Port A and the DIN Port B/C approaches. Keep3.8 mm of nominal case engagement
# above the Core+pad rear plane Z5.2. Rear seats remain unchanged; all end guides also lower to Z9.
for x in [25.3,82.7]:
    for y in [7,51]:
        boolean(body,cube('Lower Core side guide for Grove connectors',(x,y,25),(2.6,14.2,32)))

# The accepted camera-at-bottom / USB-at-left orientation puts DIN Ports B/C
# at the TOP, not the right. Lower end guides too: keep a four-corner low pocket
# instead of any tall obstructions. The closed front panel retains the Core.
for x in [34,74]:
    for y in [.3,57.7]:
        boolean(body,cube('Lower Core end guide for DIN ports',(x,y,25),(16.2,2.6,32)))

# Replace the old engraving inside the back with the actual revision.
(xmin,xmax),(ymin,ymax)=label_bounds
boolean(body,cube('Restore old revision engraving',((xmin+xmax)/2,(ymin+ymax)/2,2.6),(xmax-xmin+.4,ymax-ymin+.4,.8)),'UNION')
bpy.ops.object.text_add(location=(-28,-65,2.6));label=bpy.context.object
label.name='V1.1 engraving';label.data.body='V1.1 / M3 4.4';label.data.size=3.3;label.data.extrude=.5
bpy.ops.object.convert(target='MESH')
bm=bmesh.new();bm.from_mesh(label.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.0001)
bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(label.data);bm.free()
boolean(body,label)

bm=bmesh.new();bm.from_mesh(body.data)
bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.00001)
bmesh.ops.dissolve_degenerate(bm,edges=list(bm.edges),dist=.000001)
bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(body.data)
tree=BVHTree.FromBMesh(bm);todo=set(bm.verts);parts=0
while todo:
    parts+=1;stack=[todo.pop()]
    while stack:
        v=stack.pop()
        for edge in v.link_edges:
            other=edge.other_vert(v)
            if other in todo:todo.remove(other);stack.append(other)
topology={'nonmanifold_edges':sum(not e.is_manifold for e in bm.edges),'degenerate_faces':sum(f.calc_area()<1e-9 for f in bm.faces),
          'connected_components':parts,'volume_mm3':bm.calc_volume(signed=True)}
bm.free();assert topology['nonmanifold_edges']==0 and topology['degenerate_faces']==0 and parts==1 and topology['volume_mm3']>0,topology
checks=[]
def hit(p,d):
    result=tree.ray_cast(Vector(p),Vector(d),1000)[0];assert result is not None,(p,d);return result
def check(name,value,want):
    checks.append({'feature':name,'actual_mm':round(value,5),'expected_mm':want,'pass':abs(value-want)<.01})
    assert checks[-1]['pass'],checks[-1]
def span(p,axis):
    d=[0,0,0];d[axis]=1;a=hit(p,d);d[axis]=-1;return a[axis]-hit(p,d)[axis]
check('Side hole width along upright radio',span((98.8,-35,18.5),1),13)
check('Side hole height through enclosure depth',span((98.8,-35,18.5),2),10)
assert hit((105,-35,18.5),(-1,0,0)).x<97.6
check('Top EC11 mounting bore',span((-41,68.5,18.5),0),8)
for x,y in [(-85,-36.4),(11,-36.4),(-85,35.6),(11,37)]:check(f'Paper seat{x}/{y}',hit((x,y,50),(0,0,-1)).z,34.1)
check('Paper guide width',span((-37,37,35),0),104)
check('Paper guide height',span((-85,-.4,35),1),79.5)
check('NE USB side clearance',hit((11,34.75,28.57),(0,1,0)).y-34.75,1)
check('Core pocket width at low guides',span((54,51,7),0),55)
for x in [25.3,82.7]:
    for y in [7,51]:check(f'Low side guide top{x}/{y}',hit((x,y,40),(0,0,-1)).z,9)
check('Core pocket height at low guides',span((34,29,7),1),55)
check('Core seat',hit((34,10,50),(0,0,-1)).z,4.2)
check('Former USB tie saddle area is empty',hit((0,-53,50),(0,0,-1)).z,3)
check('No old lower-wall exit',hit((0,-75,34.7),(0,1,0)).y,-70)
for x in [-92,92]:
    for y in [-62,62]:
        check(f'Lid insert{x}/{y} diameter',span((x,y,35),0),4.4)
        check(f'Lid insert{x}/{y} depth',hit((x+4,y,50),(0,0,-1)).z-hit((x,y,50),(0,0,-1)).z,6)
for axis,want in enumerate([222,140,37]):
    check(f'Overall dimension{axis}',max(v.co[axis] for v in body.data.vertices)-min(v.co[axis] for v in body.data.vertices),want)
def volume_overlap(c):
    tmp=body.copy();tmp.data=body.data.copy();bpy.context.scene.collection.objects.link(tmp)
    boolean(tmp,c,'INTERSECT',False);bm=bmesh.new();bm.from_mesh(tmp.data);vol=abs(bm.calc_volume(signed=True));bm.free()
    bpy.data.objects.remove(tmp,do_unlink=True);bpy.data.objects.remove(c,do_unlink=True);return vol
passed=volume_overlap(cube('Cable plug feed envelope +0.5 per side',(98.8,-35,18.5),(36,12.5,9)))
inserted=volume_overlap(cube('Internal USB envelope +0.5 per side',(9.5,29,28.57),(36,12.5,9)))
assert passed<.001 and inserted<.001
# Verify correctly oriented connector access. Drawing calibration: camera
# bottom, USB left; vendor X maps to enclosure X, vendor -Z to upright Y.
# Ports B/C are on top Y56, centres X43/65; they are not on right X81.
# Keep earlier side-guide bands as structural regression, not port-location proof.
port_access={}
for name,loc,size in [
    ('left side guide band',(25.5,29,23),(3,54,27.8)),
    ('right side guide band',(82.5,29,23),(3,54,27.8)),
    ('top guide band',(54,57.7,23),(54,2.8,27.8)),
    ('bottom guide band',(54,.3,23),(54,2.8,27.8)),
    ('Port A left approach',(21.5,15,28.75),(11,18,12.5)),
    ('DIN top outlet X43 approach',(43,61.55,20),(16,11.1,16)),
    ('DIN top outlet X65 approach',(65,61.55,20),(16,11.1,16)),
]:
    overlap=volume_overlap(cube(name,loc,size));assert overlap<.001,(name,overlap)
    port_access[name]={'centre_mm':loc,'size_mm':size,'overlap_mm3':overlap}
for x in [34,74]:
    for y in [10,48]:check(f'Core rear seat{x}/{y}',hit((x,y,40),(0,0,-1)).z,4.2)
    for y in [.3,57.7]:check(f'Core low end guide{x}/{y}',hit((x,y,40),(0,0,-1)).z,9)
for x in [43,65]:check(f'DIN port{x} to upper wall',hit((x,56,19),(0,1,0)).y-56,11.6)
report={'date':'2026-10-25','revision':'1.1 correctly oriented Core port clearance','body_dimensions_mm':[222,140,37],
        'cable_hole':{'side':'Right when facing front','centre_xyz_mm':[98.8,-35,18.5],'clear_opening_mm':[13,10],'corner_radius_mm':1,'mouth_chamfer_mm':.5,
                      'reason':'Feed the11.5 x8 mm USB-C housing through the closed hole; no cable cutting'},
        'epaper_shift_down_mm':3.4,'epaper_seat_from_back_mm':34.1,'top_key_bore_mm':8,
        'usb_storage':'Rejected by user. No cradle, new tie saddle or lower-rim cable notch in this bottom.',
        'all_core_guides_top_mm':9,'din_case_to_top_wall_mm':11.6,'nominal_core_rear_mm':5.2,'nominal_side_capture_mm':3.8,
        'port_access_checks':port_access,
        'port_reference':'Official cores3_dimensions.dxf, user DIN photos and reoriented local CoreS3.stl: print X54+vendorX, Y29-vendorZ, Z25.5+vendorY. Core rear5.2/front36.7. DIN outlets TOP; original side-port checks invalid. Broad corridors, not measured cable assemblies.',
        'topology':topology,'dimension_checks':checks,'plug_overlap_mm3':{'side_feed_envelope':passed,'inserted_envelope':inserted},
        'physical_fit_verified':False,'retained_hardware':'Core rear seats; all four low corner guides at Z9; ePaper bearing ledges, lid inserts and full-depth strap loops',
        'remaining_prior_feedback':'ePaper screw retention remains unmodified; actual PCB hole positions unavailable',
        'print_orientation':'Back flat on bed, open front up'}
(OUT/'geometry_check.json').write_text(json.dumps(report,indent=2)+'\n')
body.data.calc_loop_triangles()
with (OUT/'stl/01_Titanic_bottom_v11.stl').open('wb') as f:
    f.write(b'Titanic bottom v1.1, millimetres, back down; simple side USB cable hole'.ljust(80,b'\0'))
    f.write(struct.pack('<I',len(body.data.loop_triangles)))
    for t in body.data.loop_triangles:
        p=[body.data.vertices[i].co for i in t.vertices];normal=(p[1]-p[0]).cross(p[2]-p[0]).normalized()
        f.write(struct.pack('<12fH',*normal,*p[0],*p[1],*p[2],0))
scene=bpy.context.scene;scene['design_parameters']=json.dumps(report);scene['proposal_notes']='v1.1 supersedes v1.0: correct DIN top-edge port orientation, all Core corner guides low; paper/USB corrections retained'
cam=scene.camera;cam.location=(260,-260,300);cam.rotation_euler=(Vector((0,0,15))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=290
scene.render.resolution_x=1400;scene.render.resolution_y=1050;scene.cycles.samples=48
scene.render.filepath=str(OUT/'Titanic_bottom_v11.png')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Titanic_bottom_v11.blend'))
bpy.ops.render.render(write_still=True)
print(json.dumps(report,indent=2),flush=True)
