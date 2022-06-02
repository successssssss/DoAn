export interface RelayData {
    relay_id: string,
    status: string,
    soil_humidity: string,
    name?:string
}
export interface Environment {
    temperature: string
    humidity: string
}