resource "google_service_account" "pubsub_publisher_sa" {
  account_id   = "pubsublite-publisher-sa"
  display_name = "PubSub Lite Publisher Service Account"
}

locals {
    sa_roles = [
        "roles/pubsub.publisher"
    ]
}
resource "google_service_account_iam_binding" "pubsub_publisher_sa_roles" {
  count = length(local.sa_roles)
  service_account_id = google_service_account.pubsub_publisher_sa.name
  role               = local.sa_roles[count.index]

  members = []
}

resource "google_service_account_key" "pubsub_publisher_pk" {
  service_account_id = google_service_account.pubsub_publisher_sa.name
  public_key_type    = "TYPE_X509_PEM_FILE"
}

resource "google_secret_manager_secret" "pubsub_publisher_pk" {
  secret_id = "pubsub_publisher_sa_credentials"

  replication {
    automatic = true
  }
}