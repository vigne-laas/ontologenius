#ifndef DATAPROPERTYCLIENT_H
#define DATAPROPERTYCLIENT_H

#include "ontoloGenius/utility/clients/ontologyClients/OntologyClient.h"

class DataPropertyClient : public OntologyClient
{
public:
  DataPropertyClient(ros::NodeHandle* n, const std::string& name) : OntologyClient(n, (name == "") ? "data_property" : "data_property/" + name)
  {
  }

  std::vector<std::string> getDown(const std::string& name, int depth = -1);
  std::vector<std::string> getDisjoint(const std::string& name);
  std::vector<std::string> getDomain(const std::string& name);
  std::vector<std::string> getRange(const std::string& name);

private:

};

#endif
