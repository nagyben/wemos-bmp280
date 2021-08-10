resource "google_pubsub_lite_topic" "garden_weather" {
  name = "villebon-garden-weather"
  project = data.google_project.project.number
  region = var.region
  zone = var.zone

  partition_config {
    count = 1
  }

  retention_config {
    per_partition_bytes = 1073741824 # 1GiB
  }
}