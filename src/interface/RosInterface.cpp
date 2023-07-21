#include <thread>

#include <ros/callback_queue.h>

#include "ontologenius/OntologeniusExplanation.h"

#include "ontologenius/interface/RosInterface.h"

#include "ontologenius/utils/String.h"
#include "ontologenius/core/utility/error_code.h"
#include "ontologenius/graphical/Display.h"

#define PUB_QUEU_SIZE 1000
#define SUB_QUEU_SIZE 10000

#define FEEDER_DEFAULT_RATE 20
#define FEEDER_COPY_RATE 4000

namespace ontologenius {

RosInterface::RosInterface(const std::string& name) :
                                                  reasoners_(name),
                                                  feeder_echo_(getTopicName("insert_echo", name), getTopicName("insert_explanations", name)),
#ifdef ONTO_TEST
                                                  end_feed_(true),
#endif
                                                  run_(true),
                                                  feeder_rate_(FEEDER_DEFAULT_RATE),
                                                  feeder_end_pub_(n_.advertise<std_msgs::String>(getTopicName("end", name), PUB_QUEU_SIZE)),
                                                  display_(true)
{
  onto_ = new Ontology();
  onto_->setDisplay(display_);
  reasoners_.link(onto_);
  feeder_.link(onto_);
  sparql_.link(onto_);

  name_ = name;
  n_.setCallbackQueue(&callback_queue_);
}

RosInterface::RosInterface(RosInterface& other, const std::string& name) :
                                                reasoners_(name),
                                                feeder_echo_(getTopicName("insert_echo", name), getTopicName("insert_explanations", name)),
#ifdef ONTO_TEST
                                                end_feed_(true),
#endif
                                                run_(true),
                                                feeder_rate_(FEEDER_DEFAULT_RATE),
                                                feeder_end_pub_(n_.advertise<std_msgs::String>(getTopicName("end", name), PUB_QUEU_SIZE)),
                                                display_(true)
{
  other.lock();
  onto_ = new Ontology(*other.onto_);
  onto_->setDisplay(display_);
  other.release();

  reasoners_.link(onto_);
  feeder_.link(onto_);
  feeder_.setVersioning(true);
  sparql_.link(onto_);

  name_ = name;
  n_.setCallbackQueue(&callback_queue_);
}

RosInterface::~RosInterface()
{
  lock();
  delete onto_;
}

void RosInterface::init(const std::string& lang, const std::string& intern_file, const std::vector<std::string>& files, const std::string& config_path)
{
  onto_->setLanguage(lang);
  std::string dedicated_intern_file = intern_file;
  if(name_ != "")
  {
    size_t pose = dedicated_intern_file.find(".owl");
    if(pose != std::string::npos)
      dedicated_intern_file.insert(pose, "_" + name_);
  }

  if(onto_->preload(dedicated_intern_file) == false)
    for(auto& file : files)
      onto_->readFromFile(file);

  reasoners_.configure(config_path);
  reasoners_.load();
  Display::info("Plugins loaded : " + reasoners_.list());

  feeder_rate_ = FEEDER_DEFAULT_RATE;
}

void RosInterface::init(const std::string& lang, const std::string& config_path)
{
  onto_->setLanguage(lang);

  reasoners_.configure(config_path);
  reasoners_.load();
  Display::info("Plugins loaded : " + reasoners_.list());

  feeder_.activateVersionning(true);
  feeder_rate_ = FEEDER_COPY_RATE;
}

void RosInterface::run()
{
  ros::Subscriber knowledge_subscriber = n_.subscribe(getTopicName("insert"), PUB_QUEU_SIZE, &RosInterface::knowledgeCallback, this);
  (void)knowledge_subscriber;

  ros::Subscriber stamped_knowledge_subscriber = n_.subscribe(getTopicName("insert_stamped"), PUB_QUEU_SIZE, &RosInterface::stampedKnowledgeCallback, this);
  (void)stamped_knowledge_subscriber;

  // Start up ROS service with callbacks
  ros::ServiceServer service = n_.advertiseService(getTopicName("actions"), &RosInterface::actionsHandle, this);
  (void)service;

  ros::ServiceServer service_class = n_.advertiseService(getTopicName("class"), &RosInterface::classHandle, this);
  (void)service_class;

  ros::ServiceServer service_object_property = n_.advertiseService(getTopicName("object_property"), &RosInterface::objectPropertyHandle, this);
  (void)service_object_property;

  ros::ServiceServer service_data_property = n_.advertiseService(getTopicName("data_property"), &RosInterface::dataPropertyHandle, this);
  (void)service_data_property;

  ros::ServiceServer service_individual = n_.advertiseService(getTopicName("individual"), &RosInterface::individualHandle, this);
  (void)service_individual;

  ros::ServiceServer service_reasoner = n_.advertiseService(getTopicName("reasoner"), &RosInterface::reasonerHandle, this);
  (void)service_reasoner;

  std::thread feed_thread(&RosInterface::feedThread, this);
  std::thread periodic_reasoning_thread(&RosInterface::periodicReasoning, this);

  ros::ServiceServer service_sparql = n_.advertiseService(getTopicName("sparql"), &RosInterface::sparqlHandle, this);
  (void)service_sparql;

  if(name_ != "")
    Display::info(name_ + " is ready");
  else
    Display::info("Ontologenius is ready");

  while (ros::ok() && isRunning())
  {
    callback_queue_.callAvailable(ros::WallDuration(0.1));
  }

  periodic_reasoning_thread.join();
  feed_thread.join();
}

void RosInterface::stop()
{
  run_ = false;
  callback_queue_.disable();
  callback_queue_.clear();
}

void RosInterface::lock()
{
  feeder_mutex_.lock();
  reasoner_mutex_.lock();
}

void RosInterface::release()
{
  reasoner_mutex_.unlock();
  feeder_mutex_.unlock();
}

void RosInterface::close()
{
  onto_->close();
  reasoners_.initialize();
  reasoners_.runPostReasoners();
}

void RosInterface::setDisplay(bool display)
{
  display_ = display;
  onto_->setDisplay(display_);
}

/***************
*
* Callbacks
*
****************/

void RosInterface::knowledgeCallback(const std_msgs::String::ConstPtr& msg)
{
  auto time = ros::Time::now();
  feeder_.store(msg->data, {time.sec, time.nsec});
}

void RosInterface::stampedKnowledgeCallback(const ontologenius::StampedString::ConstPtr& msg)
{
  feeder_.store(msg->data, {msg->stamp.sec, msg->stamp.nsec});
}

bool RosInterface::actionsHandle(ontologenius::OntologeniusService::Request &req,
                                 ontologenius::OntologeniusService::Response &res)
{
  res.code = 0;

  removeUselessSpace(req.action);
  removeUselessSpace(req.param);

  if(req.action == "add")
  {
    ros::service::waitForService("ontologenius/rest", -1);
    res.code = onto_->readFromUri(req.param);
  }
  else if(req.action == "fadd")
    res.code = onto_->readFromFile(req.param);
  else if(req.action == "save")
    onto_->save(req.param);
  else if(req.action == "export")
    feeder_.exportToXml(req.param);
  else if(req.action == "close")
    close();
  else if(req.action == "reset")
  {
    lock();
    delete onto_;
    onto_ = new Ontology();
    onto_->setDisplay(display_);
    reasoners_.link(onto_);
    feeder_.link(onto_);
    sparql_.link(onto_);
    release();
  }
  else if(req.action == "setLang")
    onto_->setLanguage(req.param);
  else if(req.action == "getLang")
    res.values.push_back(onto_->getLanguage());
  else
    res.code = UNKNOW_ACTION;

  return true;
}

bool RosInterface::reasonerHandle(ontologenius::OntologeniusService::Request &req,
                                ontologenius::OntologeniusService::Response &res)
{
  res.code = 0;

  reasoner_mutex_.lock();
  if(req.action == "activate")
    res.code = reasoners_.activate(req.param);
  else if(req.action == "deactivate")
    res.code = reasoners_.deactivate(req.param);
  else if(req.action == "list")
    res.values = reasoners_.listVector();
  else if(req.action == "activeList")
    res.values = reasoners_.activeListVector();
  else if(req.action == "getDescription")
    res.values.push_back(reasoners_.getDescription(req.param));
  else
    res.code = UNKNOW_ACTION;
  reasoner_mutex_.unlock();

  return true;
}

/***************
*
* Threads
*
***************/

void RosInterface::feedThread()
{
  ros::Publisher feeder_publisher = n_.advertise<std_msgs::String>(getTopicName("feeder_notifications"), PUB_QUEU_SIZE);
  bool feeder_end = true;
#ifdef ONTO_TEST
    end_feed_ = false;
#endif

  ros::Rate wait(feeder_rate_);
  while((ros::ok()) && (onto_->isInit(false) == false) && (run_ == true))
  {
    wait.sleep();
  }

  auto time = ros::Time::now();
  feeder_.store("[add]myself|", {time.sec, time.nsec});
  if(name_ != "")
    feeder_.store("[add]myself|=|" + name_, {time.sec, time.nsec});
  feeder_mutex_.lock();
  feeder_.run();
  feeder_mutex_.unlock();

  std_msgs::String msg;
  while(ros::ok() && (run_ == true))
  {
    feeder_mutex_.lock();
    bool run = feeder_.run();
    if((run == true) && (run_ == true))
    {
      if(feeder_end == true)
        feeder_end = false;

      std::vector<std::string> notifications = feeder_.getNotifications();
      for(auto notif : notifications)
      {
        Display::error("[Feeder]" + notif);
        if(name_ != "")
          notif = "[" + name_ + "]" + notif;
        msg.data = notif;
        feeder_publisher.publish(msg);
      }

      auto echo = feeder_.getValidRelations();
      feeder_echo_.add(echo);
      auto explanations = feeder_.getExplanations();
      feeder_echo_.add(explanations);
    }
    else if(feeder_end == false)
    {
      reasoner_mutex_.lock();
      reasoners_.runPostReasoners();
      reasoner_mutex_.unlock();
#ifdef ONTO_TEST
      end_feed_ = true;
#endif

      feeder_end = true;
      msg.data = "end";
      feeder_end_pub_.publish(msg);
    }

    if((run == true) && (run_ == true))
      feeder_echo_.publish();

    feeder_mutex_.unlock();

    if(ros::ok() && (run_ == true) && (run == false))
      wait.sleep();
  }
}

void RosInterface::periodicReasoning()
{
  ros::Publisher reasoner_publisher = n_.advertise<std_msgs::String>(getTopicName("reasoner_notifications", name_), PUB_QUEU_SIZE);
  ros::Publisher explanations_pub = n_.advertise<ontologenius::OntologeniusExplanation>(getTopicName("insert_explanations", name_), PUB_QUEU_SIZE);

  ros::Rate wait(10);
  while((ros::ok()) && (onto_->isInit(false) == false) && (run_ == true))
  {
    wait.sleep();
  }

  reasoner_mutex_.lock();
  reasoners_.getExplanations(); // flush explanations of initialization
  reasoner_mutex_.unlock();

  ros::Rate r(100);
  std_msgs::String msg;
  ontologenius::OntologeniusExplanation expl_msg;
  while(ros::ok() && (run_ == true))
  {
    reasoner_mutex_.lock();
    reasoners_.runPeriodicReasoners();

    auto notifications = reasoners_.getNotifications();
    for(auto& notif : notifications)
    {
      switch(notif.first)
      {
        case notification_debug: std::cout << "[Reasoners]" << notif.second << std::endl; break;
        case notification_info: Display::info("[Reasoners]" + notif.second); break;
        case notification_warning: Display::warning("[Reasoners]" + notif.second); break;
        case notification_error: Display::error("[Reasoners]" + notif.second); break;
      }
      msg.data = notif.second;
      reasoner_publisher.publish(msg);
    }

    auto explanations = reasoners_.getExplanations();
    for(auto& explanation : explanations)
    {
      expl_msg.fact = explanation.first;
      expl_msg.cause = explanation.second;
      explanations_pub.publish(expl_msg);
    }

    reasoner_mutex_.unlock();
    if(ros::ok() && (run_ == true))
      r.sleep();
  }
}

/***************
*
* Utility
*
****************/

void RosInterface::removeUselessSpace(std::string& text)
{
  while((text[0] == ' ') && (text.size() != 0))
    text.erase(0,1);

  while((text[text.size() - 1] == ' ') && (text.size() != 0))
    text.erase(text.size() - 1,1);
}

void RosInterface::set2string(const std::unordered_set<std::string>& word_set, std::string& result)
{
  for(const std::string& it : word_set)
    result += it + " ";
}

void RosInterface::set2vector(const std::unordered_set<std::string>& word_set, std::vector<std::string>& result)
{
  std::copy(word_set.begin(), word_set.end(), std::back_inserter(result));
}

param_t RosInterface::getParams(const std::string& param)
{
  param_t parameters;
  std::vector<std::string> str_params = split(param, " ");

  if(str_params.size())
    parameters.base = str_params[0];

  for(size_t i = 1; i < str_params.size(); i++)
  {
    if((str_params[i] == "-d") || (str_params[i] == "--depth"))
    {
      i++;
      int tmp = -1;
      if(sscanf(str_params[i].c_str(), "%d", &tmp) != 1)
        tmp = -1;
      parameters.depth = tmp;
    }
    else if((str_params[i] == "-s") || (str_params[i] == "--selector"))
    {
      i++;
      parameters.selector = str_params[i];
    }
    else if((str_params[i] == "-t") || (str_params[i] == "--threshold"))
    {
      i++;
      float tmp = -1;
      if(sscanf(str_params[i].c_str(), "%f", &tmp) != 1)
        tmp = -1;
      parameters.threshold = tmp;
    }
    else if((str_params[i] == "-i") || (str_params[i] == "--take_id"))
    {
      i++;
      bool tmp = false;
      if(str_params[i] == "true")
        tmp = true;

      parameters.take_id = tmp;
    }
    else if(str_params[i].size() && str_params[i][0] == '-')
    {
      Display::warning("[WARNING] unknow parameter \"" + str_params[i] + "\"");
      i++;
    }
    else if(str_params[i].size())
      parameters.base += " " + str_params[i];
  }

  return parameters;
}

} // namespace ontologenius