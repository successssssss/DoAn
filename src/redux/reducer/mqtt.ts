import { AnyAction } from "redux"
import { Environment, RelayData } from "../../model/MqttData"
import { ENVIRONMENT_DATA, RELAY_DATA } from "../type"

export interface MqttData {
    relay: RelayData[],
    environment: Environment
}

export const initEnv: Environment = {
    temperature: "16",
    humidity: "63"
}

export const initRelay: RelayData[] = [
    {
        relay_id: "1",
        status: "0",
        soil_humidity: "68"
    },
    {
        relay_id: "2",
        status: "0",
        soil_humidity: "60"
    },
    {
        relay_id: "3",
        status: "0",
        soil_humidity: "30"
    },
    {
        relay_id: "4",
        status: "0",
        soil_humidity: "0"
    }
]

export const initialMqtt = {
    relay: initRelay,
    environment: initEnv
}

const mqttReducer = (state: MqttData = JSON.parse(JSON.stringify(initialMqtt)), action: AnyAction) => {
    const { type, payload } = action
    switch (type) {
        case RELAY_DATA: {
            console.log('keytest1', payload);
            const newState = JSON.parse(JSON.stringify(state))
            const newRelay = newState.relay.find(item => {
               return Number(item.relay_id) === payload.relay_id})
            if(newRelay) {
                console.log('keytest123', newRelay);
                newRelay.status = payload.status
            }
            return newState
        }
        case ENVIRONMENT_DATA: {
            console.log('keytestenv',payload);
            const newState = JSON.parse(JSON.stringify(state))
            newState.environment = payload
            return newState
        }
        default:
            return state;
    }
}
export default mqttReducer