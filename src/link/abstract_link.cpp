#include "link/abstract_link.hpp"

AbstractLink::AbstractLink(QObject* const parent)
    : QObject(parent)
{
}

void AbstractLink::reconnect()
{
}

void AbstractLink::switchLink()
{
}
