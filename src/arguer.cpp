#include "ontoloGenius/ontoGraphs/Ontology.h"

#include "ontologenius/standard_service.h"
#include "ontoloGenius/utility/error_code.h"

#include <iostream>
#include "ros/ros.h"

#include "ontoloGenius/Computer.h"

#include "ontoloGenius/Parser.h"

using namespace std;

Ontology onto;

bool reference_handle(ontologenius::standard_service::Request  &req,
                      ontologenius::standard_service::Response &res)
{
  bool done = false;
  res.value = "";
  res.code = 0;

  if(req.action == "add")
  {
    res.code = onto.readFromUri(req.param);
  }
  else if(req.action == "close")
  {
    onto.close();
  }
  else if(req.action == "reset")
  {
    onto = Ontology();
  }
  else if(req.action == "test")
  {
    Computer comp;
    if(comp.compute(req.param, onto.classes_))
      res.value = "true";
    else
      res.value = "false";
    //comp.compute("red_cube|young_animal=color_animal|age_object", onto);
  }
  else
    res.code = UNKNOW_ACTION;

  return true;
}

bool class_handle(ontologenius::standard_service::Request  &req,
                  ontologenius::standard_service::Response &res)
{
  bool done = false;
  res.value = "";
  res.code = 0;

if(req.action == "getDown")
  {
    set<string> entities = onto.classes_.getDown(req.param);
    string result = "";
    for(set<string>::iterator it = entities.begin(); it != entities.end(); ++it)
      result += *it + " ";
    res.value = result;
  }
  else if(req.action == "getUp")
  {
    set<string> entities = onto.classes_.getUp(req.param);
    string result = "";
    for(set<string>::iterator it = entities.begin(); it != entities.end(); ++it)
      result += *it + " ";
    res.value = result;
  }
  else if(req.action == "getDisjoint")
  {
    set<string> entities = onto.classes_.getDisjoint(req.param);
    string result = "";
    for(set<string>::iterator it = entities.begin(); it != entities.end(); ++it)
      result += *it + " ";
    res.value = result;
  }
  else
    res.code = UNKNOW_ACTION;

  return true;
}

bool property_handle(ontologenius::standard_service::Request  &req,
                    ontologenius::standard_service::Response &res)
{
  bool done = false;
  res.value = "";
  res.code = 0;

if(req.action == "getDown")
  {
    set<string> entities = onto.properties_.getDown(req.param);
    string result = "";
    for(set<string>::iterator it = entities.begin(); it != entities.end(); ++it)
      result += *it + " ";
    res.value = result;
  }
  else if(req.action == "getUp")
  {
    set<string> entities = onto.properties_.getUp(req.param);
    string result = "";
    for(set<string>::iterator it = entities.begin(); it != entities.end(); ++it)
      result += *it + " ";
    res.value = result;
  }
  else if(req.action == "getDisjoint")
  {
    set<string> entities = onto.properties_.getDisjoint(req.param);
    string result = "";
    for(set<string>::iterator it = entities.begin(); it != entities.end(); ++it)
      result += *it + " ";
    res.value = result;
  }
  else if(req.action == "getInverse")
  {
    set<string> entities = onto.properties_.getInverse(req.param);
    string result = "";
    for(set<string>::iterator it = entities.begin(); it != entities.end(); ++it)
      result += *it + " ";
    res.value = result;
  }
  else if(req.action == "getDomain")
  {
    set<string> entities = onto.properties_.getDomain(req.param);
    string result = "";
    for(set<string>::iterator it = entities.begin(); it != entities.end(); ++it)
      result += *it + " ";
    res.value = result;
  }
  else if(req.action == "getRange")
  {
    set<string> entities = onto.properties_.getRange(req.param);
    string result = "";
    for(set<string>::iterator it = entities.begin(); it != entities.end(); ++it)
      result += *it + " ";
    res.value = result;
  }
  else
    res.code = UNKNOW_ACTION;

  return true;
}

int main(int argc, char** argv)
{
  ros::init(argc, argv, "ontoloGenius");

  ros::NodeHandle n;

  ros::service::waitForService("ontoloGenius/REST", -1);

  for(unsigned int i = 1; i < argc; i++)
  {
    OntologyReader reader(onto);
    reader.readFromFile(string(argv[i]));
  }

  // Start up ROS service with callbacks
  ros::ServiceServer service = n.advertiseService("ontoloGenius/actions", reference_handle);
  ros::ServiceServer serviceClass = n.advertiseService("ontoloGenius/class", class_handle);
  ros::ServiceServer serviceProperty = n.advertiseService("ontoloGenius/property", property_handle);
  ROS_DEBUG("ontoloGenius ready");

  std::string code = "";
  code += "var::man += fablab.isIn() - (bob + max);\n";
  code += "if(adult == age) \n";
  code += "{";
  code += "//this is a comment\n";
  code += "\tif(age == adult)\n";
  code += "\t\tont::print(var::man);\n";
  code += "}\n";
  code += "else if(old == /* */age)\n";
  code += "\tif(young == age)\n";
  code += "\t\tont::print(\"this is an else if\");\n";
  code += "/*\n";
  code += "an other comment*/ \n";
  code += "ont::print(\"this is the else \");\n\n";
  code += "function(var::men - bob);\n";
  code += "var::men =if(var::man == man);\n";

  Error error;

  Code my_code(code);
  Parser p(&my_code);

  error.cpy(p.getError());
  error.printStatus();

  ros::spin();

  ROS_DEBUG("KILL ontoloGenius");

  return 0;
}
