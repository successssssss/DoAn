import { RegisterDevice } from "../model/Register"
import { getApiService } from "./BaseApiService"

export const registerDevice = async (params: RegisterDevice) => {
    const endpoint = `api/device`
    const axios = await getApiService()
    return axios.post(endpoint, params)
}

export const getMacAddressByUserEmail = async (email: string) => {
    const param = {
        user_email: email
    }
    const endpoint = `api/mac-address`
    const axios = await getApiService()
    return axios.post(endpoint, param)
}

export const saveDeviceToken = async (params: {device_token: string, email: string}) => {
    const endpoint = `api/save-token`
    const axios = await getApiService()
    return axios.post(endpoint, params)
}