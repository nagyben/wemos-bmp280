variable "env_suffix" {
  type    = string
  default = ""
}

variable "ci_sa" {
  type        = string
  description = "The id of the CI service account"
}

variable "project" {
  type        = string
  description = "The GCP project name"
}

variable "mqtt_tester_enabled" {
  type        = bool
  description = "Whether or not to create MQTT testing client credentials"
}