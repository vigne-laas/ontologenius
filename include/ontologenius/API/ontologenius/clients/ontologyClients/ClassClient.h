#ifndef ONTOLOGENIUS_CLASSCLIENT_H
#define ONTOLOGENIUS_CLASSCLIENT_H

#include "ontologenius/API/ontologenius/clients/ontologyClients/OntologyClient.h"

class ClassClient : public OntologyClient
{
public:
  ClassClient(ros::NodeHandle* n, const std::string& name) : OntologyClient(n, (name == "") ? "class" : "class/" + name)
  {
  }

  std::vector<std::string> getDown(const std::string& name, int depth = -1, const std::string& selector = "");
  std::vector<std::string> getDisjoint(const std::string& name);

  std::vector<std::string> getOn(const std::string& name, const std::string& property, const std::string& selector = "");
  std::vector<std::string> getFrom(const std::string& property, const std::string& name, const std::string& selector = "");
  std::vector<std::string> getWith(const std::string& class_from, const std::string& class_to, const std::string& selector = "", int depth = -1);

  std::vector<std::string> getRelatedFrom(const std::string& property);
  std::vector<std::string> getRelatedOn(const std::string& property);
  std::vector<std::string> getRelatedWith(const std::string& name);

  std::vector<std::string> getRelationFrom(const std::string& name, int depth = -1);
  std::vector<std::string> getRelationOn(const std::string& name, int depth = -1);
  std::vector<std::string> getRelationWith(const std::string& name);

  std::vector<std::string> getDomainOf(const std::string& name, const std::string& selector = "", int depth = -1);
  std::vector<std::string> getRangeOf(const std::string& name, const std::string& selector = "", int depth = -1);

private:

};

#endif // ONTOLOGENIUS_CLASSCLIENT_H