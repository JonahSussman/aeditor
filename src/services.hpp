#pragma once

// Service locator. VLC and Script are both services that this can locate. Falls
// back to a null service just in case one hasn't been loaded yet. To learn more
// about the service locator pattern, I'd reccomend checking this article out
// to start: https://gameprogrammingpatterns.com/service-locator.html

#include "services/service_vlc.hpp"
#include "services/service_script.hpp"

#include "services/locator.hpp"
