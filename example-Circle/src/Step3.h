
#pragma once


void step_three_setup(ofApp* app)
{
    auto orbit_points = app->orbits.setup(app->orbit_circle).
        distinct_until_changed().
        start_with(true).
        map(
            [=](bool orbits){
                if (orbits) {
                    return app->orbitPointsFromTimeInPeriod(
                        app->timeInPeriodFromMilliseconds(
                            app->updates.milliseconds())).
                        as_dynamic();
                } else {
                    return rxcpp::observable<>::
                        just(ofPoint(0,0)).
                        as_dynamic();
                }
            }).
        switch_on_next();
    
    auto location_points = app->locations.setup(app->show_circle).
        distinct_until_changed().
        start_with(true).
        map(
            [=](bool locations){
                if (locations) {
                    return app->mouse.moves().
                        map(ofApp::pointFromMouse).
                        as_dynamic();
                } else {
                    return rxcpp::observable<>::
                        never<ofPoint>().
                        as_dynamic();
                }
            }).
        switch_on_next().
        start_with(ofPoint(ofGetWidth()/2, ofGetHeight()/2));
    
    location_points.
        combine_latest(std::plus<>(), orbit_points).
        subscribe(
            [=](ofPoint c){
                // update the point that the draw() call will use
                app->center = c;
            });
}
