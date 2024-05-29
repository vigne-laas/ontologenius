#ifndef ONTOLOGENIUS_REASONERNONE_H
#define ONTOLOGENIUS_REASONERNONE_H

#include "ontologenius/core/reasoner/plugins/ReasonerInterface.h"

namespace ontologenius {

  class ReasonerNone : public ReasonerInterface
  {
  public:
    ReasonerNone() = default;
    virtual ~ReasonerNone() = default;

    virtual bool preReason(const QueryInfo_t& query_info) override;
    virtual void postReason() override;
    virtual bool periodicReason() override;

    virtual bool implementPreReasoning() override { return true; }
    virtual bool implementPostReasoning() override { return true; }
    virtual bool implementPeriodicReasoning() override { return true; }

    virtual std::string getName() override;
    virtual std::string getDescription() override;
  };

} // namespace ontologenius

#endif // ONTOLOGENIUS_REASONERNONE_H
