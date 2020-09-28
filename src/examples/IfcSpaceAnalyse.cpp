/*
 * Filename: d:\Project_Cpp\IfcOpenShell\src\examples\IfcSpaceAnalyse.cpp
 * Path: d:\Project_Cpp\IfcOpenShell\src\examples
 * Created Date: Monday, July 13th 2020, 4:00:37 pm
 * Author: WouRaoyu
 * 
 * Copyright (c) 2020 VrLab
 */

#include <string>
#include <iostream>
#include <fstream>

#include <TColgp_Array2OfPnt.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColStd_Array1OfInteger.hxx>

#include <Geom_BSplineSurface.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakeHalfSpace.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepTopAdaptor_FClass2d.hxx>
#include <Standard_Version.hxx>

#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>

#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <BRep_Tool.hxx>
#include <GC_MakePlane.hxx>
#include <Extrema_ExtPC.hxx>
#include <TopExp.hxx>
#include <TopAbs.hxx>
#include <TopoDS.hxx>

#include "StlAPI.hxx"

#include "../ifcparse/Ifc2x3.h"
#define IfcSchema Ifc2x3

// #include "../ifcparse/Ifc4.h"
// #define IfcSchema Ifc4


#include "../ifcparse/IfcBaseClass.h"
#include "../ifcparse/IfcHierarchyHelper.h"
#include "../ifcgeom/IfcGeom.h"
#include "../ifcgeom/IfcGeomTree.h"
#include "../ifcgeom_schema_agnostic/Serialization.h"

#if USE_VLD
#include <vld.h>
#endif

enum FACETYPE
{
	FACE_XMIN,
	FACE_XMAX,
	FACE_YMIN,
	FACE_YMAX,
	FACE_ZMIN,
	FACE_ZMAX
};

TopoDS_Shape get_bbox_shape(Bnd_Box bbox);
void init_bbox(std::vector<TopoDS_Shape>& shapes);
void fast_generate_rooms(std::vector<TopoDS_Shape>& shapes);
void recursiveConnection(IfcParse::IfcFile& file, int id, std::vector<int>& connects_id, std::vector<TopoDS_Shape>& shapes);
TopoDS_Shape helper_fn_create_shape(IfcGeom::IteratorSettings& settings, IfcUtil::IfcBaseClass* instance, IfcUtil::IfcBaseClass* representation = 0);

static IfcGeom::IteratorSettings settings;

int main(int argc, char** argv) {
	if (argc != 2) {
		std::cout << "usage: IfcParseExamples <filename.ifc>" << std::endl;
		return 1;
	}

	// Redirect the output (both progress and log) to stdout
	// Logger::SetOutput(&std::cout, &std::cout);

	// Parse the IFC file provided in argv[1]
	IfcParse::IfcFile file(argv[1]);
	if (!file.good()) {
		std::cout << "Unable to parse .ifc file" << std::endl;
		return 1;
	}

	settings.set(IfcGeom::IteratorSettings::APPLY_DEFAULT_MATERIALS, true);
	settings.set(IfcGeom::IteratorSettings::USE_WORLD_COORDS, true);
	settings.set(IfcGeom::IteratorSettings::WELD_VERTICES, true);
	settings.set(IfcGeom::IteratorSettings::DISABLE_OPENING_SUBTRACTIONS, true);

	std::vector<int> connects_id;
	std::vector<std::vector<int>> connects_register;
	std::vector<std::vector<TopoDS_Shape>> connects_shapes;

	size_t begin_index, end_index = 0;
	IfcSchema::IfcWall::list::ptr walls = file.instances_by_type<IfcSchema::IfcWall>();

	/* generate relations between walls */

	for (IfcSchema::IfcWall::list::it it = walls->begin(); it != walls->end(); it++)
	{
		const IfcSchema::IfcWall* wall = *it;

		int id_wall = wall->data().id();
		if (std::find(connects_id.begin(), connects_id.end(), id_wall) != connects_id.end()) continue;

		std::vector<TopoDS_Shape> shapes;

		recursiveConnection(file, id_wall, connects_id, shapes);
		
		begin_index = end_index;
		end_index = connects_id.size();

		if (end_index == begin_index) continue;

		std::vector<int> connects_group;
		for (size_t i = begin_index; i < end_index; i++)
		{
	 		connects_group.emplace_back(connects_id[i]);
	 		std::cout << connects_id[i] << " ";
		}

		for (size_t i = 0; i < shapes.size(); i++)
		{
			TopoDS_Shape shape = shapes[i];
			BRepMesh_IncrementalMesh meshMaker(shape, 0.01, Standard_False);
			meshMaker.Perform();
			std::string file_name = "group_" + std::to_string(connects_shapes.size()) + "wall_" + std::to_string(i) + ".stl";
			StlAPI::Write(shape, file_name.c_str());
		}

		std::cout << std::endl << "------------------------------" << std::endl;
		connects_register.emplace_back(connects_group);
		connects_shapes.emplace_back(shapes);
	}

	/*for (IfcSchema::IfcWall::list::it it = walls->begin(); it != walls->end(); it++) {
		const IfcSchema::IfcWall* wall = *it;

		const IfcSchema::IfcCurtainWall* curtain = wall->as<IfcSchema::IfcCurtainWall>();

		int id_wall = wall->data().id();

		TopoDS_Shape shape = helper_fn_create_shape(settings, file.instance_by_id(id_wall));
		
		std::string file_name = "wall_" + std::to_string(id_wall) + ".stl";

		if (shape.NbChildren() > 1)
		{
			std::cout << id_wall << " has subshapes " << shape.NbChildren() << std::endl;
			Bnd_Box bbox;
			BRepBndLib::Add(shape, bbox);
			Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
			bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
			shape = get_bbox_shape(bbox);
			file_name = "wall_" + std::to_string(id_wall) + "_bbox.stl";
		}
		BRepMesh_IncrementalMesh meshMaker(shape, 0.01, Standard_False);
		meshMaker.Perform();

		StlAPI::Write(shape, file_name.c_str());
	}
	

	IfcSchema::IfcSlab::list::ptr slabs = file.instances_by_type<IfcSchema::IfcSlab>();
	for (IfcSchema::IfcSlab::list::it it = slabs->begin(); it != slabs->end(); it++) {
		const IfcSchema::IfcSlab* slab = *it;
		int id_slab = slab->data().id();

		TopoDS_Shape shape = helper_fn_create_shape(settings, file.instance_by_id(id_slab));

		std::string file_name = "slab_" + std::to_string(id_slab) + ".stl";

		if (shape.NbChildren() > 1)
		{
			std::cout << id_slab << " has subshapes " << shape.NbChildren() << std::endl;
			Bnd_Box bbox;
			BRepBndLib::Add(shape, bbox);
			Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
			bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
			shape = get_bbox_shape(bbox);
			file_name = "slab_" + std::to_string(id_slab) + "_bbox.stl";
		}
		BRepMesh_IncrementalMesh meshMaker(shape, 0.01, Standard_False);
		meshMaker.Perform();

		StlAPI::Write(shape, file_name.c_str());
	}*/

	//std::vector<TopoDS_Shape> shapes = connects_shapes[0];
	//fast_generate_rooms(shapes);
}

void recursiveConnection(IfcParse::IfcFile& file, int id, std::vector<int>& connects_id, std::vector<TopoDS_Shape>& shapes) {
	IfcUtil::IfcBaseClass* entity = file.instance_by_id(id);
	IfcSchema::IfcWall *wall = entity->as<IfcSchema::IfcWall>();

	IfcSchema::IfcRelConnectsElements::list::ptr relcon_to;
	IfcSchema::IfcRelConnectsElements::list::ptr relcon_from;

	if (!wall)
	{
		IfcSchema::IfcBuildingElement *element = entity->as<IfcSchema::IfcBuildingElement>();
		relcon_to = element->ConnectedTo();
		relcon_from = element->ConnectedFrom();
	}
	else
	{
		relcon_to = wall->ConnectedTo();
		relcon_from = wall->ConnectedFrom();
	}
	
	if (std::find(connects_id.begin(), connects_id.end(), id) != connects_id.end()) return;

	if (!relcon_to.get()->size() && !relcon_from.get()->size()) return;

	TopoDS_Shape shape = helper_fn_create_shape(settings, entity);
	
	if (shape.IsNull()) return;

	if (shape.Closed())
	{
		std::cout << wall->data().id() << " is not closed" << std::endl;
	}

	shapes.emplace_back(shape);
	connects_id.emplace_back(id);

	if (relcon_to.get()->size()) {
		for (IfcSchema::IfcRelConnectsElements::list::it it_rel = relcon_to->begin(); it_rel != relcon_to->end(); it_rel++)
		{
			const IfcSchema::IfcRelConnectsElements *rel = *it_rel;
			int id_to = rel->RelatedElement()->data().id();
			if (id_to) recursiveConnection(file, id_to, connects_id, shapes);
		}
	}

	if (relcon_from.get()->size()) {
		for (IfcSchema::IfcRelConnectsElements::list::it it_rel = relcon_from->begin(); it_rel != relcon_from->end(); it_rel++)
		{
			const IfcSchema::IfcRelConnectsElements *rel = *it_rel;
			int id_from = rel->RelatingElement()->data().id();
			if (id_from) recursiveConnection(file, id_from, connects_id, shapes);
		}
	}
}

TopoDS_Shape helper_fn_create_shape(IfcGeom::IteratorSettings& settings, IfcUtil::IfcBaseClass* instance, IfcUtil::IfcBaseClass* representation) {
	IfcParse::IfcFile* file = instance->data().file;

	IfcGeom::Kernel kernel(file);
	kernel.setValue(IfcGeom::Kernel::GV_MAX_FACES_TO_ORIENT, settings.get(IfcGeom::IteratorSettings::SEW_SHELLS) ? std::numeric_limits<double>::infinity() : -1);
	kernel.setValue(IfcGeom::Kernel::GV_DIMENSIONALITY, (settings.get(IfcGeom::IteratorSettings::INCLUDE_CURVES) ? (settings.get(IfcGeom::IteratorSettings::EXCLUDE_SOLIDS_AND_SURFACES) ? -1. : 0.) : +1.));
	kernel.setValue(IfcGeom::Kernel::GV_LAYERSET_FIRST, settings.get(IfcGeom::IteratorSettings::LAYERSET_FIRST) ? +1.0 : -1.0);

	if (instance->declaration().is(IfcSchema::IfcProduct::Class())) {
		if (representation) {
			if (!representation->declaration().is(IfcSchema::IfcRepresentation::Class())) {
				std::cout << "Supplied representation not of type IfcRepresentation" << std::endl;
				return TopoDS_Shape();
			}
		}

		typename IfcSchema::IfcProduct* product = (typename IfcSchema::IfcProduct*) instance;

		if (!representation && !product->hasRepresentation()) {
			std::cout << product->data().id() << " Representation is NULL" << std::endl;
			return TopoDS_Shape();
		}

		typename IfcSchema::IfcProductRepresentation* prodrep = product->Representation();
		typename IfcSchema::IfcRepresentation::list::ptr reps = prodrep->Representations();
		typename IfcSchema::IfcRepresentation* ifc_representation = (typename IfcSchema::IfcRepresentation*) representation;

		if (!ifc_representation) {
			// First, try to find a representation based on the settings
			for (typename IfcSchema::IfcRepresentation::list::it it = reps->begin(); it != reps->end(); ++it) {
				typename IfcSchema::IfcRepresentation* rep = *it;
				if (!rep->hasRepresentationIdentifier()) {
					continue;
				}
				if (!settings.get(IfcGeom::IteratorSettings::EXCLUDE_SOLIDS_AND_SURFACES)) {
					if (rep->RepresentationIdentifier() == "Body") {
						ifc_representation = rep;
						break;
					}
				}
				if (settings.get(IfcGeom::IteratorSettings::INCLUDE_CURVES)) {
					if (rep->RepresentationIdentifier() == "Plan" || rep->RepresentationIdentifier() == "Axis") {
						ifc_representation = rep;
						break;
					}
				}
			}
		}

		// Otherwise, find a representation within the 'Model' or 'Plan' context
		if (!ifc_representation) {
			for (typename IfcSchema::IfcRepresentation::list::it it = reps->begin(); it != reps->end(); ++it) {
				typename IfcSchema::IfcRepresentation* rep = *it;
				typename IfcSchema::IfcRepresentationContext* context = rep->ContextOfItems();

				// TODO: Remove redundancy with IfcGeomIterator.h
				if (context->hasContextType()) {
					std::set<std::string> context_types;
					if (!settings.get(IfcGeom::IteratorSettings::EXCLUDE_SOLIDS_AND_SURFACES)) {
						context_types.insert("model");
						context_types.insert("design");
						context_types.insert("model view");
						context_types.insert("detail view");
					}
					if (settings.get(IfcGeom::IteratorSettings::INCLUDE_CURVES)) {
						context_types.insert("plan");
					}

					std::string context_type_lc = context->ContextType();
					for (std::string::iterator c = context_type_lc.begin(); c != context_type_lc.end(); ++c) {
						*c = tolower(*c);
					}
					if (context_types.find(context_type_lc) != context_types.end()) {
						ifc_representation = rep;
					}
				}
			}
		}

		if (!ifc_representation) {
			if (reps->size()) {
				// Return a random representation
				ifc_representation = *reps->begin();
			}
			else {
				std::cout << "No suitable IfcRepresentation found" << std::endl;
				return TopoDS_Shape();
			}
		}

		IfcGeom::BRepElement<double>* brep = kernel.convert(settings, ifc_representation, product);
		if (!brep) {
			std::cout << "Failed to process shape" << std::endl;
			return TopoDS_Shape();
		}
		else {
			return brep->geometry().as_compound();
		}
	}
	else {
		if (!representation) {
			if (instance->declaration().is(IfcSchema::IfcRepresentationItem::Class()) || instance->declaration().is(IfcSchema::IfcRepresentation::Class())) {
				IfcGeom::IfcRepresentationShapeItems shapes = kernel.convert(instance);

				IfcGeom::ElementSettings element_settings(settings, kernel.getValue(IfcGeom::Kernel::GV_LENGTH_UNIT), instance->declaration().name());
				IfcGeom::Representation::BRep brep(element_settings, boost::lexical_cast<std::string>(instance->data().id()), shapes);
				try {
					return brep.as_compound();
				}
				catch (...) {
					std::cout << "Error during shape serialization" << std::endl;
					return TopoDS_Shape();
				}
			}
		}
		else {
			std::cout << "Invalid additional representation specified" << std::endl;
			return TopoDS_Shape();
		}
	}
}

gp_XYZ get_bbox_center(Bnd_Box bbox) {
	Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
	bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
	Standard_Real xcenter = (xmin + xmax) / 2;
	Standard_Real ycenter = (ymin + ymax) / 2;
	Standard_Real zcenter = (zmin + zmax) / 2;
	return gp_XYZ(xcenter, ycenter, zcenter);
}

gp_XYZ get_face_center(Bnd_Box bbox, FACETYPE type) {
	Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
	bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
	Standard_Real xcenter = (xmin + xmax) / 2;
	Standard_Real ycenter = (ymin + ymax) / 2;
	Standard_Real zcenter = (zmin + zmax) / 2;
	if (type == FACE_XMIN) return gp_XYZ(xmin, ycenter, zcenter);
	else if (type == FACE_XMAX) return gp_XYZ(xmax, ycenter, zcenter);
	else if (type == FACE_YMIN) return gp_XYZ(xcenter, ymin, zcenter);
	else if (type == FACE_YMAX) return gp_XYZ(xcenter, ymax, zcenter);
	else if (type == FACE_ZMIN) return gp_XYZ(xcenter, ycenter, zmin);
	else if (type == FACE_ZMAX) return gp_XYZ(xcenter, ycenter, zmax);
}

gp_XYZ get_bbox_dimension(Bnd_Box bbox) {
	Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
	bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
	Standard_Real xlength = xmax - xmin;
	Standard_Real ywidth = ymax - ymin;
	Standard_Real zheight = zmax - zmin;
	return gp_XYZ(xlength, ywidth, zheight);
}

TopoDS_Shape get_bbox_shape(Bnd_Box bbox) {
	Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
	bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
	return BRepPrimAPI_MakeBox(gp_Pnt(xmin, ymin, zmin), gp_Pnt(xmax, ymax, zmax)).Shape();
}

// 根据墙的特性 返回面积最大的一组面对
std::pair<TopoDS_Face, TopoDS_Face> determine_face_pair(TopoDS_Shape wall) {
	TopExp_Explorer exp(wall, TopAbs_FACE);
	double max_area = std::numeric_limits<double>::min();
	std::pair<TopoDS_Face, TopoDS_Face> face_pair;
	while (exp.More()) {
		TopoDS_Face face = TopoDS::Face(exp.Current());
		GProp_GProps prop;
		BRepGProp::SurfaceProperties(face, prop);
		double area = prop.Mass();
		exp.Next();
		if (max_area < area) continue;
		if (max_area > area) {
			face_pair.first = face;
		} else {
			face_pair.second = face;
		}
	}
}

TopoDS_Face determine_face(std::pair<TopoDS_Face, TopoDS_Face> face_pair, gp_XYZ center) {
	return TopoDS_Face();
}

void init_bbox(std::vector<TopoDS_Shape>& shapes) {
	Bnd_Box bbox;
	for (std::vector<TopoDS_Shape>::const_iterator it = shapes.begin(); it != shapes.end(); it++)
	{
		TopoDS_Shape shape = *it;
		BRepBndLib::Add(shape, bbox);
	}
	Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
	bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

	double wall_thick = 0.40;
	gp_Pnt pnt_min(xmin + wall_thick, ymin + wall_thick, zmin);
	gp_Pnt pnt_max(xmax - wall_thick, ymax - wall_thick, zmax);
	Bnd_Box bbox_offset(pnt_min, pnt_max);

	// 对每面墙构造其boundingbox 并存储
	std::vector<TopoDS_Shape> shapes_outer;
	std::vector<TopoDS_Shape> shapes_inner;
	std::vector<TopoDS_Shape> shapes_inter;
	for (std::vector<TopoDS_Shape>::const_iterator it = shapes.begin(); it != shapes.end(); it++)
	{
		Bnd_Box bbox_it;
		TopoDS_Shape shape = *it;
		BRepBndLib::AddClose(shape, bbox_it);
		TopoDS_Shape bbox_shape = get_bbox_shape(bbox_it);
		if (bbox_offset.IsOut(bbox_it)) {
			shapes_outer.emplace_back(bbox_shape);
		}
		else {
			gp_Pnt it_min = bbox_it.CornerMin();
			gp_Pnt it_max = bbox_it.CornerMax();
			if (bbox_offset.IsOut(it_min) && bbox_offset.IsOut(it_max))
			{
				shapes_inter.emplace_back(bbox_shape);
			}
			else {
				shapes_inner.emplace_back(bbox_shape);
			}
		}
	}

	// 情况一
	// 如果存在横断整个空间的 
	// 通过平行的墙 构建bbox
	// 针对bbox 分别判断剩余的墙分割在哪里
	// 依次进行这种操作
	// gp_Pnt it_min = bbox_it.CornerMin();
	// gp_Pnt it_max = bbox_it.CornerMax();

	// if (bbox_offset.IsOut(it_min) && bbox_offset.IsOut(it_max))
	// {
	// }

	TopoDS_Shape shape = shapes[0];
	TopExp_Explorer exp(shape, TopAbs_FACE);
	
	Geom_Plane *plane;
	TopoDS_Face face;

	
	while (exp.More())
	{
		face = TopoDS::Face(exp.Current());
		Handle_Geom_Surface surf = BRep_Tool::Surface(face); // 将拓扑topo转为几何geom
		plane = Handle_Geom_Plane::DownCast(surf).get();
		exp.Next();
	}

	// 获取Z轴方向
	Standard_Real axis_z = plane->Axis().Direction().Z();

	// 获取面法向量
	gp_XYZ face_normal = plane->Axis().Direction().XYZ();

	// 计算room对应的bbox的中心点
	Bnd_Box bbox_room;
	gp_XYZ room_center = get_bbox_center(bbox_room);

	//We compute some volumetric properties of the resulting shape
	GProp_GProps volumeProperties;
	BRepGProp::VolumeProperties(shape, volumeProperties);

	// 计算TopoDS_Face面对应的包围盒中心点
	Bnd_Box bbox_face;
	BRepBndLib::Add(face, bbox_face);
	gp_XYZ face_center = get_bbox_center(bbox_face);

	// 获取指向房屋中心的向量 并做规范化处理
	gp_XYZ vertex_face2center = room_center - face_center;
	vertex_face2center.Normalize();

	// 将面的法向量 与 指向房屋中心的向量相乘 判断方向
	Standard_Real result = vertex_face2center.Dot(face_normal);

	// 如果方向完全相反为-1 显然不可能完全相反
	// 所以此处以0.8为阈值进行判断

	std::vector<TopoDS_Solid> halfspaces;

	//if (result < -0.8) {
	//  gp_Pln face_plane = plane->Pln(); //获得half space
	//	TopoDS_Face new_face = BRepBuilderAPI_MakeFace(face_plane).Face();
	//	TopoDS_Solid halfspace = BRepPrimAPI_MakeHalfSpace(new_face, room_center).Solid();
	//	halfspaces.emplace_back(halfspace);
	//}

	//TopoDS_Shape common_shape = bbox_shape;

	//for (std::vector<TopoDS_Solid>::const_iterator it; it != halfspaces.end(); it++)
	//{
	//	TopoDS_Solid halfspace = *it;
	//	TopoDS_Shape common_shape = BRepAlgoAPI_Common(common_shape, halfspace).Shape();
	//}
}

// 假设房间是正方体 且垂直与z轴 可使用该方法进行分割
// 进行这步操作之前需要先将坐标系旋转平移到原点处
void fast_generate_rooms(std::vector<TopoDS_Shape>& shapes) {
	Bnd_Box bbox;
	for (std::vector<TopoDS_Shape>::const_iterator it = shapes.begin(); it != shapes.end(); it++)
	{
		TopoDS_Shape shape = *it;
		BRepBndLib::Add(shape, bbox);
	}
	Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
	bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

	// 通过墙的厚度直接进行offset 得到的便是该层所有的空间
	double wall_thick = 0.30;
	gp_Pnt pnt_min(xmin + wall_thick, ymin + wall_thick, zmin);
	gp_Pnt pnt_max(xmax - wall_thick, ymax - wall_thick, zmax);
	Bnd_Box bbox_offset(pnt_min, pnt_max);

	TopoDS_Shape offset_shape = get_bbox_shape(bbox_offset);

	gp_XYZ bbox_dim = get_bbox_dimension(bbox_offset);

	std::vector<Bnd_Box> bbox_inner;
	for (std::vector<TopoDS_Shape>::const_iterator it = shapes.begin(); it != shapes.end(); it++)
	{
		Bnd_Box bbox_it;
		TopoDS_Shape shape = *it;
		BRepBndLib::AddClose(shape, bbox_it);
		if (!bbox_offset.IsOut(bbox_it)) {
			bbox_inner.emplace_back(bbox_it);
		}
	}

	std::cout << bbox_dim.X() << " and " << bbox_dim.Y() << std::endl;

	// 对每面墙构造其boundingbox 并存储
	for (std::vector<Bnd_Box>::const_iterator it = bbox_inner.begin(); it != bbox_inner.end(); it++)
	{
		Bnd_Box bbox_it = *it;
		gp_XYZ it_dim = get_bbox_dimension(bbox_it);
		std::cout << it_dim.X() << " and " << it_dim.Y() << std::endl;
		if (it_dim.X() < bbox_dim.X() && it_dim.Y() < bbox_dim.Y()) continue;
		else {
			if (it_dim.X() > it_dim.Y())
			{
				gp_XYZ center_ymin = get_face_center(bbox_it, FACE_YMIN);
				gp_XYZ center_ymax = get_face_center(bbox_it, FACE_YMAX);
				gp_XYZ ref_ymin = get_face_center(bbox_offset, FACE_YMIN);
				gp_XYZ ref_ymax = get_face_center(bbox_offset, FACE_YMAX);

				gp_Dir dir = gp_Dir(0.0, 1.0, 0.0);

				gp_Pln plane_ymin = gp_Pln(center_ymin, dir);
				gp_Pln plane_ymax = gp_Pln(center_ymax, dir);
				
				TopoDS_Face face_ymin = BRepBuilderAPI_MakeFace(plane_ymin).Face();
				TopoDS_Face face_ymax = BRepBuilderAPI_MakeFace(plane_ymax).Face();

				TopoDS_Solid halfspace_ymin = BRepPrimAPI_MakeHalfSpace(face_ymin, ref_ymin).Solid();
				TopoDS_Shape shape_ymin = BRepAlgoAPI_Common(offset_shape, halfspace_ymin).Shape();

				TopoDS_Solid halfspace_ymax = BRepPrimAPI_MakeHalfSpace(face_ymax, ref_ymax).Solid();
				TopoDS_Shape shape_ymax = BRepAlgoAPI_Common(offset_shape, halfspace_ymax).Shape();

				std::cout << "Success" << std::endl;
			} else {
				gp_XYZ center_xmin = get_face_center(bbox_it, FACE_XMIN);
				gp_XYZ center_xmax = get_face_center(bbox_it, FACE_XMAX);
				gp_XYZ ref_xmin = get_face_center(bbox_offset, FACE_XMIN);
				gp_XYZ ref_xmax = get_face_center(bbox_offset, FACE_XMAX);

				gp_Dir dir = gp_Dir(1.0, 0.0, 0.0);

				gp_Pln plane_xmin = gp_Pln(center_xmin, dir);
				gp_Pln plane_xmax = gp_Pln(center_xmax, dir);

				TopoDS_Face face_xmin = BRepBuilderAPI_MakeFace(plane_xmin).Face();
				TopoDS_Face face_xmax = BRepBuilderAPI_MakeFace(plane_xmax).Face();

				TopoDS_Solid halfspace_xmin = BRepPrimAPI_MakeHalfSpace(face_xmin, ref_xmin).Solid();
				TopoDS_Shape shape_ymin = BRepAlgoAPI_Common(offset_shape, halfspace_xmin).Shape();

				TopoDS_Solid halfspace_xmax = BRepPrimAPI_MakeHalfSpace(face_xmax, ref_xmax).Solid();
				TopoDS_Shape shape_ymax = BRepAlgoAPI_Common(offset_shape, halfspace_xmax).Shape();

				std::cout << "Success" << std::endl;
			}
		}
	}
	
}

std::pair<double, double> lim_edge_length(const TopoDS_Shape& a) {
	double min_edge_len = std::numeric_limits<double>::max();
	double max_edge_len = std::numeric_limits<double>::min();

	TopExp_Explorer exp(a, TopAbs_EDGE);
	for (; exp.More(); exp.Next()) {
		GProp_GProps prop;
		BRepGProp::LinearProperties(exp.Current(), prop);
		double l = prop.Mass();
		if (l < min_edge_len) {
			min_edge_len = l;
		}
		if (l > max_edge_len)
		{
			max_edge_len = l;
		}
	}
	std::pair<double, double> lim_edge_len(min_edge_len, max_edge_len);
	return lim_edge_len;
}

double min_vertex_edge_distance(const TopoDS_Shape& a, double min_search, double max_search) {
	double M = std::numeric_limits<double>::infinity();

	TopTools_IndexedMapOfShape vertices, edges;

	TopExp::MapShapes(a, TopAbs_VERTEX, vertices);
	TopExp::MapShapes(a, TopAbs_EDGE, edges);

	IfcGeom::impl::tree<int> tree;

	// Add edges to tree
	for (int i = 1; i <= edges.Extent(); ++i) {
		tree.add(i, edges(i));
	}

	for (int j = 1; j <= vertices.Extent(); ++j) {
		const TopoDS_Vertex& v = TopoDS::Vertex(vertices(j));
		gp_Pnt p = BRep_Tool::Pnt(v);

		Bnd_Box b;
		b.Add(p);
		b.Enlarge(max_search);

		std::vector<int> edge_idxs = tree.select_box(b, false);
		std::vector<int>::const_iterator it = edge_idxs.begin();
		for (; it != edge_idxs.end(); ++it) {
			const TopoDS_Edge& e = TopoDS::Edge(edges(*it));
			TopoDS_Vertex v1, v2;
			TopExp::Vertices(e, v1, v2);

			if (v.IsSame(v1) || v.IsSame(v2)) {
				continue;
			}

			BRepAdaptor_Curve crv(e);
			Extrema_ExtPC ext(p, crv);
			if (!ext.IsDone()) {
				continue;
			}

			for (int i = 1; i <= ext.NbExt(); ++i) {
				const double m = sqrt(ext.SquareDistance(i));
				if (m < M && m > min_search) {
					M = m;
				}
			}
		}
	}

	return M;
}

class points_on_planar_face_generator {
private:
	const TopoDS_Face& f_;
	Handle(Geom_Surface) plane_;
	BRepTopAdaptor_FClass2d cls_;
	double u0, u1, v0, v1;
	int i, j;
	bool inset_;
	static const int N = 10;

public:
	points_on_planar_face_generator(const TopoDS_Face& f, bool inset = false)
		: f_(f)
		, plane_(BRep_Tool::Surface(f_))
		, cls_(f_, BRep_Tool::Tolerance(f_))
		, i((int)inset), j((int)inset)
		, inset_(inset)
	{
		BRepTools::UVBounds(f_, u0, u1, v0, v1);
	}

	void reset() {
		i = j = (int)inset_;
	}

	bool operator()(gp_Pnt& p) {
		while (j < N) {
			double u = u0 + (u1 - u0) * i / N;
			double v = v0 + (v1 - v0) * j / N;

			i++;
			if (i == N) {
				i = 0;
				j++;
			}

			// Specifically does not consider ON
			if (cls_.Perform(gp_Pnt2d(u, v)) == TopAbs_IN) {
				plane_->D0(u, v, p);
				return true;
			}
		}

		return false;
	}
};

bool faces_overlap(const TopoDS_Face& f, const TopoDS_Face& g) {
	points_on_planar_face_generator pgen(f);

	BRep_Builder B;
	gp_Pnt test;
	double eps = BRep_Tool::Tolerance(f) + BRep_Tool::Tolerance(g);

	BRepExtrema_DistShapeShape x;
	x.LoadS1(g);

	while (pgen(test)) {
		TopoDS_Vertex V;
		B.MakeVertex(V, test, Precision::Confusion());
		x.LoadS2(V);
		x.Perform();
		if (x.IsDone() && x.NbSolution() == 1) {
			if (x.Value() > eps) {
				return false;
			}
		}
	}

	return true;
}

void bbox_overlap(double p, const TopoDS_Shape& a, const TopTools_ListOfShape& b, TopTools_ListOfShape& c) {
	Bnd_Box A;
	BRepBndLib::Add(a, A);

	if (A.IsVoid()) {
		return;
	}

	TopTools_ListIteratorOfListOfShape it(b);
	for (; it.More(); it.Next()) {
		Bnd_Box B;
		BRepBndLib::Add(it.Value(), B);

		if (B.IsVoid()) {
			continue;
		}

		if (A.Distance(B) < p) {
			c.Append(it.Value());
		}
	}
}

double min_face_face_distance(const TopoDS_Shape& a, double max_search) {
	/*
	NB: This is currently only implemented for planar surfaces.
	*/
	double M = std::numeric_limits<double>::infinity();

	TopTools_IndexedMapOfShape faces;

	TopExp::MapShapes(a, TopAbs_FACE, faces);

	IfcGeom::impl::tree<int> tree;

	// Add faces to tree
	for (int i = 1; i <= faces.Extent(); ++i) {
		if (BRep_Tool::Surface(TopoDS::Face(faces(i)))->DynamicType() == STANDARD_TYPE(Geom_Plane)) {
			tree.add(i, faces(i));
		}
	}

	for (int j = 1; j <= faces.Extent(); ++j) {
		const TopoDS_Face& f = TopoDS::Face(faces(j));
		const Handle(Geom_Surface)& fs = BRep_Tool::Surface(f);

		if (fs->DynamicType() != STANDARD_TYPE(Geom_Plane)) {
			continue;
		}

		points_on_planar_face_generator pgen(f);

		Bnd_Box b;
		BRepBndLib::AddClose(f, b);
		b.Enlarge(max_search);

		std::vector<int> face_idxs = tree.select_box(b, false);
		std::vector<int>::const_iterator it = face_idxs.begin();
		for (; it != face_idxs.end(); ++it) {
			if (*it == j) {
				continue;
			}

			const TopoDS_Face& g = TopoDS::Face(faces(*it));
			const Handle(Geom_Surface)& gs = BRep_Tool::Surface(g);

			auto p0 = Handle(Geom_Plane)::DownCast(fs);
			auto p1 = Handle(Geom_Plane)::DownCast(gs);

			if (p0->Position().IsCoplanar(p1->Position(), max_search, asin(max_search))) {
				pgen.reset();

				BRepTopAdaptor_FClass2d cls(g, BRep_Tool::Tolerance(g));

				gp_Pnt test;
				while (pgen(test)) {
					gp_Vec d = test.XYZ() - p1->Position().Location().XYZ();
					double u = d.Dot(p1->Position().XDirection());
					double v = d.Dot(p1->Position().YDirection());

					// nb: TopAbs_ON is explicitly not considered to prevent matching adjacent faces
					// with similar orientations.
					if (cls.Perform(gp_Pnt2d(u, v)) == TopAbs_IN) {
						gp_Pnt test2;
						p1->D0(u, v, test2);
						double w = std::abs(gp_Vec(p1->Position().Direction().XYZ()).Dot(test2.XYZ() - test.XYZ()));
						if (w < M) {
							M = w;
						}
					}
				}
			}
		}
	}

	return M;
}
