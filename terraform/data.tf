data "google_project" "project" {
  project_id = var.project
  depends_on = [google_project_service.enabled_apis]
}