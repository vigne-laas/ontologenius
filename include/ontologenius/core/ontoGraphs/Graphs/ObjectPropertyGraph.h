#ifndef ONTOLOGENIUS_OBJECTROPERTYGRAPH_H
#define ONTOLOGENIUS_OBJECTROPERTYGRAPH_H

#include <string>
#include <vector>
#include <unordered_set>
#include <map>
#include <stdint.h>

#include "ontologenius/core/ontoGraphs/Graphs/OntoGraph.h"

#include "ontologenius/core/ontoGraphs/Branchs/ObjectPropertyBranch.h"
#include "ontologenius/core/ontoGraphs/Branchs/ClassBranch.h"

namespace ontologenius {

struct ObjectPropertyVectors_t
{
   std::vector<Single_t<std::string>> mothers_;
   std::vector<Single_t<std::string>> disjoints_;
   std::vector<Single_t<std::string>> inverses_;
   std::vector<Single_t<std::string>> domains_;
   std::vector<Single_t<std::string>> ranges_;
   std::vector<std::vector<std::string>> chains_;
   Properties_t properties_;
   std::map<std::string, std::vector<std::string>> dictionary_;
   std::map<std::string, std::vector<std::string>> muted_dictionary_;
   bool annotation_usage_;

   ObjectPropertyVectors_t() : annotation_usage_(false) {}
};

//for friend
class ObjectPropertyDrawer;
class IndividualGraph;
class AnonymousClassGraph;

//for graphs usage
class ClassGraph;

class ObjectPropertyGraph : public OntoGraph<ObjectPropertyBranch_t>
{
  friend ObjectPropertyDrawer;
  friend IndividualGraph;
  friend ClassGraph;
  friend AnonymousClassGraph;
public:
  explicit ObjectPropertyGraph(IndividualGraph* individual_graph, ClassGraph* class_graph);
  ObjectPropertyGraph(const ObjectPropertyGraph& other, IndividualGraph* individual_graph, ClassGraph* class_graph);
  ~ObjectPropertyGraph() {}

  void deepCopy(const ObjectPropertyGraph& other);

  virtual void close() override;

  ObjectPropertyBranch_t* add(const std::string& value, ObjectPropertyVectors_t& property_vectors);
  void add(std::vector<std::string>& disjoints);

  std::unordered_set<std::string> getDisjoint(const std::string& value);
  std::unordered_set<index_t> getDisjoint(index_t value);
  void getDisjointPtr(ObjectPropertyBranch_t* branch, std::unordered_set<ObjectPropertyBranch_t*>& res);
  std::unordered_set<std::string> getInverse(const std::string& value);
  std::unordered_set<index_t> getInverse(index_t value);
  std::unordered_set<std::string> getDomain(const std::string& value);
  std::unordered_set<index_t> getDomain(index_t value);
  void getDomainPtr(ObjectPropertyBranch_t* branch, std::unordered_set<ClassBranch_t*>& res, size_t depth = -1);
  std::unordered_set<std::string> getRange(const std::string& value);
  std::unordered_set<index_t> getRange(index_t value);
  void getRangePtr(ObjectPropertyBranch_t* branch, std::unordered_set<ClassBranch_t*>& res, size_t depth = -1);

  void createInvertChains();

  bool addInverseOf(const std::string& from, const std::string& on);
  bool removeInverseOf(const std::string& from, const std::string& on);

  bool isIrreflexive(const std::string& prop);
  bool isIrreflexive(ObjectPropertyBranch_t* prop);
  bool isAsymetric(const std::string& prop);
  bool isAsymetric(ObjectPropertyBranch_t* prop);

private:
  ClassGraph* class_graph_;

  void cpyBranch(ObjectPropertyBranch_t* old_branch, ObjectPropertyBranch_t* new_branch);
  void cpyChainOfBranch(ObjectPropertyBranch_t* old_branch, ObjectPropertyBranch_t* new_branch);

  std::vector<std::vector<ObjectPropertyBranch_t*>> getInvertChains(const std::vector<ObjectPropertyBranch_t*>& partial_res, const std::vector<ObjectPropertyBranch_t*>& chain);
};

} // namespace ontologenius

#endif // ONTOLOGENIUS_OBJECTROPERTYGRAPH_H
