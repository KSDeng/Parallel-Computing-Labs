
#ifndef CS3210_ASSIGNMENT1_MAIN_H
#define CS3210_ASSIGNMENT1_MAIN_H

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <deque>
using namespace std;

using adjacency_matrix = vector<std::vector<size_t>>;

enum MRT_LINE {
    MRT_LINE_GREEN,
    MRT_LINE_YELLOW,
    MRT_LINE_BLUE
};

enum TRAIN_STATUS {
    /* static status begin */
    TRAIN_STATUS_INITIAL,
    TRAIN_STATUS_IN_PLATFORM,
    TRAIN_STATUS_QUEUEING_FOR_PLATFORM,
    TRAIN_STATUS_OPENING_DOOR,
    TRAIN_STATUS_LOADING_PASSENGERS,
    TRAIN_STATUS_WAITING_FOR_LINK,
    TRAIN_STATUS_WAITING_FOR_ANOTHER_TICK,
    /* static status end */

    /* transitioning status begin */
    TRAIN_STATUS_TRANSITIONING
    /* transitioning status end */
};

enum DIRECTION {
    DIRECTION_FORWARD,
    DIRECTION_BACKWARD
};

class Train;
class Station;
class Platform;
class Link;
class LineStationsManager;
class TimeCounter;

vector<Station*> green_line;
vector<Station*> yellow_line;
vector<Station*> blue_line;
LineStationsManager *green_line_manager;
LineStationsManager *yellow_line_manager;
LineStationsManager *blue_line_manager;

class Train {
public:
    Train(unsigned id, MRT_LINE line, Station *location);

    /* getters begin */
    unsigned getId() {
        return this->train_id;
    }

    TRAIN_STATUS currentStatus() {
        return this->status;
    }

    void setCurrentStatus(TRAIN_STATUS s) {
        this->status = s;
    }

    DIRECTION currentDirection() {
        return this->direction;
    }

    Station *currentStation() {     // used only when train is static
        return this->station_at;
    }

    Platform *currentPlatform() {   // used only when train is static
        return this->platform_at;
    }

    Link *currentLink() {       // used only when train is transitioning
        return this->link_at;
    }

    LineStationsManager *lineStationsManager() {
        switch (this->line) {
            case MRT_LINE_GREEN: return green_line_manager;
            case MRT_LINE_YELLOW: return yellow_line_manager;
            case MRT_LINE_BLUE: return blue_line_manager;
            default: return nullptr;
        }
    }

    TimeCounter *getLoadingCounter() {
        return this->load_passengers_counter;
    }

    TimeCounter *getTravelingCounter() {
        return this->travel_link_counter;
    }
    /* getters end */

    /* core functions begin */
    void enterPlatform(Platform *plt);

    void enterPlatformQueue(Platform *plt);

    void leavePlatform(Platform *plt);

    void openDoor();

    void loadPassengersFromStation(Station *st);

    void waitForAnotherTikToLink(Link *link);

    void waitForLink();

    void enterLink(Link *link);

    void leaveLink(Link *link);

    void transition();

    void turnAround();
    /* core functions end */

    /* print utils begin */
    string currentInfo();
    /* print utils end */

private:
    unsigned train_id;
    MRT_LINE line;

    /* status begin */
    TRAIN_STATUS status;
    Station *station_at;
    Platform *platform_at;
    Link *link_at;          // used only when transitioning
    DIRECTION direction;
    /* status end */

    /* counters begin */
    TimeCounter *load_passengers_counter;
    TimeCounter *travel_link_counter;
    /* counters end */
};

class Platform {
public:
    Platform() = default;
    Platform(Station *st_belong, Station *st_head_to) {
        this->st_belong = st_belong;
        this->st_head_to = st_head_to;
        occupied = false;
    }

    bool isOccupied() {
        return this->occupied;
    }

    void setOccupied(bool o) {
        this->occupied = o;
    }

    Station *getStation() {
        return st_belong;
    }

    Station *getDstStation() {
        return st_head_to;
    }

private:
    Station *st_belong;
    Station *st_head_to;
    bool occupied;
    deque<Train*> holding_area;

    void addTrainToHoldingArea(Train *train) {
        this->holding_area.push_back(train);
    }
    friend class Train;
};

class Station {
public:
    Station() = default;
    explicit Station(string& name) {
        this->name = name;
    }

    string getName() {
        return this->name;
    }

    unsigned getPopularity() {
        return this->popularity;
    }

    void setPop(size_t pop) {
        this->popularity = pop;
    }

    void addLinkTowards(Station *dst, Link *link) {
        this->links[dst->name] = link;
        this->platforms[dst->name] = new Platform(this, dst);
    }

    /* core functions begin */
    Platform *getTargetPlatform(Station *dst_station) {     // manage platforms and links
        return this->platforms[dst_station->name];
    }

    Link *getTargetLink(Station *dst_station) {
        return this->links[dst_station->name];
    }
    /* core functions end */

private:
    string name;
    unsigned popularity;
    unordered_map<string, Platform*> platforms;         // dst station name -> platform
    unordered_map<string, Link*> links;                 // dst station name -> link
};

class Link {
public:
    Link(Station *from, Station *to, unsigned dist) {
        this->st_from = from;
        this->st_to = to;
        this->distance = dist;

        this->occupied = false;
    }

    bool isOccupied() {
        return this->occupied;
    }

    void setOccupied(bool o) {
        this->occupied = o;
    }

    unsigned getDistance() {
        return distance;
    }

    Station *getSrcStation() {
        return st_from;
    }

    Station *getDstStation() {
        return st_to;
    }

private:
    Station *st_from;
    Station *st_to;
    unsigned distance;
    bool occupied;
};

class LineStationsManager {
public:
     explicit LineStationsManager(MRT_LINE line) {
        switch (line) {
            case MRT_LINE_GREEN: this->line_stations = &green_line; break;
            case MRT_LINE_YELLOW: this->line_stations = &yellow_line; break;
            case MRT_LINE_BLUE: this->line_stations = &blue_line; break;
            default: break;
        }

        for (unsigned i = 0; i < this->line_stations->size(); ++i) {
            station_index[(*this->line_stations)[i]->getName()] = i;
        }
    }

    bool isTerminalStation(Station *st) {
        return this->line_stations->back() == st;
    }

    bool isStartingStation(Station *st) {
        return this->line_stations->front() == st;
    }

    Station *getNextStationOfDirection(Station *cur_station, DIRECTION direction) {
        return direction == DIRECTION_FORWARD ?
               getNextStationInLine(cur_station) : getPreviousStationInLine(cur_station);
    }

private:
    Station *getNextStationInLine(Station *cur_station) {
        unsigned index = station_index[cur_station->getName()];
        return (*line_stations)[index + 1];
    }

    Station *getPreviousStationInLine(Station *cur_station) {
        unsigned index = station_index[cur_station->getName()];
        return (*line_stations)[index - 1];
    }

    vector<Station*>* line_stations;
    unordered_map<string, unsigned> station_index;
};

class TimeCounter {
public:
    TimeCounter() {
        this->time_to_count = 0;
    }

    void setCounter(unsigned count) {
        this->time_to_count = count;
    }

    void count() {
        this->time_to_count--;
    }

    bool finish() {
        return this->time_to_count == 0;
    }

private:
    unsigned time_to_count;
};

#endif //CS3210_ASSIGNMENT1_MAIN_H
