resource "google_app_engine_application" "app_engine" {
  project       = var.project
  location_id   = var.region
  database_type = "CLOUD_FIRESTORE"
  depends_on    = [google_project_service.enabled_apis]
}