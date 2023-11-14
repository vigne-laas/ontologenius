#include "ontologenius/core/ontoGraphs/Graphs/AnonymousClassGraph.h"

#include <algorithm>
#include <iostream>

#include "ontologenius/core/ontoGraphs/Graphs/ClassGraph.h"
#include "ontologenius/core/ontoGraphs/Graphs/DataPropertyGraph.h"
#include "ontologenius/core/ontoGraphs/Graphs/IndividualGraph.h"
#include "ontologenius/core/ontoGraphs/Graphs/ObjectPropertyGraph.h"

#include "ontologenius/utils/String.h"

namespace ontologenius {


AnonymousClassGraph::AnonymousClassGraph(ClassGraph* class_graph, ObjectPropertyGraph* object_property_graph, 
DataPropertyGraph* data_property_graph, IndividualGraph* individual_graph)
{
  class_graph_ = class_graph;
  object_property_graph_ = object_property_graph;
  data_property_graph_ = data_property_graph;
  individual_graph_ = individual_graph;
  cpt_anonymous_classes_ = 0;
}

AnonymousClassGraph::AnonymousClassGraph(const AnonymousClassGraph& other, ClassGraph* class_graph, ObjectPropertyGraph* object_property_graph, DataPropertyGraph* data_property_graph, IndividualGraph* individual_graph)
{
  class_graph_ = class_graph;
  individual_graph_ = individual_graph;
  object_property_graph_ = object_property_graph;
  data_property_graph_ = data_property_graph;
}

AnonymousClassElement_t* AnonymousClassGraph::createElement(ExpressionMember_t* exp_leaf)
{
  AnonymousClassElement_t* ano_element = new AnonymousClassElement_t();
  std::vector<std::string> vect_equiv = exp_leaf->rest.getRestrictionVector();
  ano_element->nb_sub = exp_leaf->nb_sub;

  if(exp_leaf->logical_type_ != logical_none)
  {
    ano_element->logical_type_ = exp_leaf->logical_type_;
    return ano_element;
  }
  else if(exp_leaf->oneof == true)
  {
    ano_element->oneof = true;
    return ano_element;
  }
  // class, indiv or literal node only
  if(vect_equiv.size() == 1)
  {
    std::string s = vect_equiv.front();
    if(isIn("http://www.w3.org/", s))
    {
      std::string type = split(s, "#").back();
      ano_element->card_.card_range_ = new LiteralNode(type, "");
    }
    else
    {
      if(exp_leaf->mother != nullptr && exp_leaf->mother->oneof)
        ano_element->individual_involved_= individual_graph_->findOrCreateBranch(s);
      else
        ano_element->class_involved_= class_graph_->findOrCreateBranch(s);
    }    
  }
  else if(vect_equiv[1] == "value")
  { 
    ano_element->object_property_involved_= object_property_graph_->findOrCreateBranch(vect_equiv.front());
    ano_element->card_.card_type_ = cardinality_value;
    ano_element->individual_involved_= individual_graph_->findOrCreateBranch(vect_equiv[2]);
  }
  else{

      std::string property = vect_equiv.front();

      if(vect_equiv[1]=="some")
        ano_element->card_.card_type_ = cardinality_some;
      else if(vect_equiv[1]=="only")
        ano_element->card_.card_type_ = cardinality_only;
      else if(vect_equiv[1]=="exactly")
        ano_element->card_.card_type_ = cardinality_exactly;
      else if(vect_equiv[1]=="min")
        ano_element->card_.card_type_ = cardinality_min;
      else if(vect_equiv[1]=="max")
        ano_element->card_.card_type_ = cardinality_max;
      else
        ano_element->card_.card_type_ = cardinality_error;

      if(exp_leaf->nb_sub == 1)
      {
        if(exp_leaf->isDataProp)
          ano_element->data_property_involved_= data_property_graph_->findOrCreateBranch(property);
        else
          ano_element->object_property_involved_= object_property_graph_->findOrCreateBranch(property);
        
        if(vect_equiv.size() == 3)
            ano_element->card_.card_number_ = std::stoi(vect_equiv.back());
      }
      else
      {
        if(vect_equiv.size() == 4)
          ano_element->card_.card_number_ = stoi(vect_equiv[2]);
        else
          ano_element->card_.card_number_ = 0;

        // if last element in vect_equiv contains http://www.w3.org/ => data property
        if(isIn("http://www.w3.org/", vect_equiv.back()))
        {
          ano_element->data_property_involved_= data_property_graph_->findOrCreateBranch(property);
          std::string type = split(vect_equiv.back(), "#").back();
          ano_element->card_.card_range_ = new LiteralNode(type, "");
        }
        else
        {
          ano_element->object_property_involved_= object_property_graph_->findOrCreateBranch(property);
          ano_element->class_involved_= class_graph_->findOrCreateBranch(vect_equiv.back());
        }
      }  
    }
  
  return ano_element;
}

void AnonymousClassGraph::update(ExpressionMember_t* exp , AnonymousClassElement_t* ano)
{
  if(exp->logical_type_ != logical_none || exp->oneof == true || exp->nb_sub == 1)
  {
    AnonymousClassElement_t* ano_tmp = createElement(exp);
    ano->sub_elements_.push_back(ano_tmp);

    for(auto elem: exp->intersects)
      update(elem, ano_tmp);
  }
  else
    ano->sub_elements_.push_back(createElement(exp));
}

AnonymousClassBranch_t* AnonymousClassGraph::add(const std::string& value, AnonymousClassVectors_t& ano)
{   
    std::lock_guard<std::shared_timed_mutex> lock(Graph<AnonymousClassBranch_t>::mutex_);

    std::string ano_name = "ano"+ std::to_string(cpt_anonymous_classes_);
    AnonymousClassBranch_t* anonymous_branch = new AnonymousClassBranch_t(ano_name);
    //std::cout << "New anonymous branch id : " << anonymous_branch->value() << " =>" << ano.str_equivalences << std::endl;
    cpt_anonymous_classes_++;

    anonymous_classes_.push_back(anonymous_branch);

    ExpressionMember_t* root = ano.equivalence;
    AnonymousClassElement_t* ano_elem = new AnonymousClassElement_t();

     // Class equivalent to 
    ClassBranch_t* class_branch = class_graph_->findOrCreateBranch(value);
    anonymous_branch->class_equiv_= class_branch;
    class_branch->equiv_relations_.push_back(anonymous_branch);

    if(root->mother == nullptr)
    {
      ExpressionMember_t* leaf = root->intersects.front();

      if(leaf->intersects.size() != 0)
      {
        update(leaf, ano_elem);
        // not so good but works to erase the empty node at the beginning
        anonymous_branch->ano_elem_ = ano_elem->sub_elements_.front();
      }
      else
        anonymous_branch->ano_elem_ = createElement(leaf);
    }
  return anonymous_branch;
}

void AnonymousClassGraph::printTreev2(AnonymousClassElement_t* ano_elem, size_t level, bool root)
{ 
  std::string space(level*4, ' ');
  std::string tmp = "";

  if(root)
    std::cout << space;

  if(ano_elem->logical_type_ == LogicalNodeType_e::logical_and)
    tmp+= "and";
  else if(ano_elem->logical_type_ == LogicalNodeType_e::logical_or)
    tmp+= "or";
  else if(ano_elem->logical_type_ == LogicalNodeType_e::logical_not)
    tmp+= "not";
  else if(ano_elem->oneof)
    tmp+= "oneOf";
  else if(ano_elem->object_property_involved_ != nullptr)
  {
    tmp += ano_elem->object_property_involved_->value();
    tmp += " " + getCardinality(ano_elem->card_.card_type_);

    if(ano_elem->card_.card_type_ == ontologenius::CardType_t::cardinality_value)
      tmp += " " + ano_elem->individual_involved_->value();
    else
    {
      if(ano_elem->card_.card_number_ != 0)
        tmp += " " +  std::to_string(ano_elem->card_.card_number_);
      if(ano_elem->class_involved_ != nullptr)
        tmp += " " + ano_elem->class_involved_->value();
    }
  }
  else if(ano_elem->data_property_involved_ != nullptr )
  {
    tmp += ano_elem->data_property_involved_->value();
    tmp += " " + getCardinality(ano_elem->card_.card_type_);
    if(ano_elem->card_.card_number_ != 0)
      tmp += " " +  std::to_string(ano_elem->card_.card_number_);
    if(ano_elem->card_.card_range_ != nullptr)
      tmp += " " + ano_elem->card_.card_range_->value();
  }
  else
  { 
    if(ano_elem->class_involved_ != nullptr)
      tmp+= ano_elem->class_involved_->value();
    else if(ano_elem->individual_involved_ != nullptr)
      tmp+= ano_elem->individual_involved_->value();
    else if(ano_elem->card_.card_range_ != nullptr)
      tmp += ano_elem->card_.card_range_->type_;
  }
  std::cout << tmp << std::endl;

  for(auto sub_elem : ano_elem->sub_elements_)
  {
    //std::cout << "│   ";

    for(int i = 0; i < int(level) ; i++)
      std::cout << "│   ";
    if(sub_elem == ano_elem->sub_elements_.back())
      std::cout << "└── ";
    else
       std::cout << "├── " ;    
    printTreev2(sub_elem, level + 1, false);
  }
}

std::string AnonymousClassGraph::getCardinality(CardType_t value)
{
  std::string res;

  switch (value)
  {
  case CardType_t::some_:
    return "some";
  case CardType_t::only_:
    return "only";
  case CardType_t::min_:
    return "min";
  case CardType_t::max_:
   return "max";
  case CardType_t::exactly_:
    return "exactly";
  case CardType_t::value_:
    return "value";
  case CardType_t::error_:
    return "error";
  default:
    return "";
  }
}
} // namespace ontologenius