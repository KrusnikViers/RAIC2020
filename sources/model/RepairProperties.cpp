#include "RepairProperties.hpp"

RepairProperties::RepairProperties() { }
RepairProperties::RepairProperties(std::vector<EntityType> validTargets, int power) : validTargets(validTargets), power(power) { }
RepairProperties RepairProperties::readFrom(InputStream& stream) {
    RepairProperties result;
    result.validTargets = std::vector<EntityType>(stream.readInt());
    for (size_t i = 0; i < result.validTargets.size(); i++) {
        switch (stream.readInt()) {
        case 0:
            result.validTargets[i] = EntityType::WALL;
            break;
        case 1:
            result.validTargets[i] = EntityType::SUPPLY;
            break;
        case 2:
            result.validTargets[i] = EntityType::BASE;
            break;
        case 3:
            result.validTargets[i] = EntityType::DRONE;
            break;
        case 4:
            result.validTargets[i] = EntityType::MBARRACKS;
            break;
        case 5:
            result.validTargets[i] = EntityType::MELEE;
            break;
        case 6:
            result.validTargets[i] = EntityType::BARRACKS;
            break;
        case 7:
            result.validTargets[i] = EntityType::RANGED;
            break;
        case 8:
            result.validTargets[i] = EntityType::RESOURCE;
            break;
        case 9:
            result.validTargets[i] = EntityType::TURRET;
            break;
        default:
            throw std::runtime_error("Unexpected tag value");
        }
    }
    result.power = stream.readInt();
    return result;
}
void RepairProperties::writeTo(OutputStream& stream) const {
    stream.write((int)(validTargets.size()));
    for (const EntityType& validTargetsElement : validTargets) {
        stream.write((int)(validTargetsElement));
    }
    stream.write(power);
}
