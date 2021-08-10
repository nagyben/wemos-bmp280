resource "google_pubsub_lite_topic" "garden_weather" {
  name    = "villebon-weather-station"
  project = data.google_project.project.number
  region  = var.region
  zone    = var.zone

  lifecycle {
    prevent_destroy = true
  }
  partition_config {
    count = 1
    capacity {
      publish_mib_per_sec   = 4
      subscribe_mib_per_sec = 4
    }
  }

  retention_config {
    per_partition_bytes = 32212254720 # 30 GiB (the minimum)
  }
}