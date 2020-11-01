/*
 * File: OffSerializer.h
 * Project: SpaceExtraction
 * File Created: Friday, 30th October 2020 4:21:18 pm
 * Author: WouRaoyu
 * Last Modified: Friday, 30th October 2020 7:30:00 pm
 * Modified By: WouRaoyu
 * Copyright 2020 vge lab
 */

#ifndef OffSERIALIZER_H
#define OffSERIALIZER_H

#include <set>
#include <string>
#include <fstream>

#include <BRepBuilderAPI_GTransform.hxx>
#include <BRepBuilderAPI_Transform.hxx>

#include "GeometrySerializer.h"
#include "../ifcgeom_schema_agnostic/IfcGeomRenderStyles.h"

typedef std::map<std::string, std::string> semanticSetting;

class OffSerializer : public GeometrySerializer {
private:
	std::ofstream off_stream; // geometry info
	std::ofstream offx_stream; // semantic info
	std::ofstream offc_stream; // connection info
	semanticSetting setting_fixed;
	std::map<int, std::set<int>> storey_fixed;
	std::map<int, std::set<int>> aggregate_fixed;
	IfcParse::IfcFile* ifc_file;
	unsigned int offLine_count;
	unsigned int vcount_total;
	short precision;

protected:
	void initSemanticSetting();
	std::string semanticName(std::string type);

public:
	OffSerializer(const std::string& out_filename, const SerializerSettings& setting);

	bool ready();
	void writeHeader() {};
	void writeMaterial(const IfcGeom::Material& style);
	void write(const IfcGeom::TriangulationElement<real_t>* o);
	void write(const IfcGeom::BRepElement<real_t>* /*o*/) {}
	void finalize();
	void generateStorey();
	void generateAggregate();
	bool isTesselated() const { return true; }
	void setUnitNameAndMagnitude(const std::string& /*name*/, float /*magnitude*/) {}
	void setFile(IfcParse::IfcFile* file) { 
		ifc_file = file;
		generateStorey();
		generateAggregate();
	}
};

#endif