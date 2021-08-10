locals {
    services = [
        "iam.googleapis.com",
        "cloudbuild.googleapis.com",
        "pubsublite.googleapis.com",
        "cloudfunctions.googleapis.com"
    ]
}

resource "google_project_service" "enabled_apis" {
  count = length(local.services)
  project = var.project
  service = local.services[count.index]

  timeouts {
    create = "5m"
    update = "5m"
  }

  disable_dependent_services = true
}