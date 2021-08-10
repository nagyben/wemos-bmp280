resource "google_service_account" "pubsub_publisher_sa" {
  project      = var.project
  account_id   = "pubsublite-publisher-sa"
  display_name = "PubSub Lite Publisher Service Account"
}

locals {
  sa_roles = [
    "roles/pubsub.publisher",
  ]
}

resource "google_project_iam_binding" "pubsub_publisher_sa_roles" {
  project = var.project
  count   = length(local.sa_roles)
  role    = local.sa_roles[count.index]

  members = [
    "serviceAccount:${google_service_account.pubsub_publisher_sa.email}",
  ]
}