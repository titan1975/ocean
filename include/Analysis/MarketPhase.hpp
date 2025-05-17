#pragma once

namespace SunTzu {
    enum class MarketPhase {
        RANGING,    // Sideways price action
        TRENDING,   // Strong directional move
        CHAOS       // High volatility, erratic moves
    };
}