resource "google_service_account_key" "pubsub_publisher_pk" {
  service_account_id = google_service_account.pubsub_publisher_sa.name
  public_key_type    = "TYPE_X509_PEM_FILE"
}

resource "google_secret_manager_secret" "pubsub_publisher_pk" {
  project   = var.project
  secret_id = "pubsub_publisher_sa_credentials"

  replication {
    automatic = true
  }
}

resource "google_secret_manager_secret_version" "pubsub_publisher_pk" {
  secret = google_secret_manager_secret.pubsub_publisher_pk.id
  secret_data = google_service_account_key.pubsub_publisher_pk.private_key
}