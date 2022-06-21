resource "google_artifact_registry_repository" "docker_repo_init" {
  provider = google-beta
  repository_id = "nagyben-cr"
  description = "Docker container registry"
  format = "DOCKER"
  location = var.location
}

resource "google_artifact_registry_repository_iam_member" "ci" {
  provider = google-beta

  location = google_artifact_registry_repository.docker_repo_init.location
  repository = google_artifact_registry_repository.docker_repo_init.name
  role   = "roles/artifactregistry.reader"
  member = "serviceAccount:${var.ci_sa}"
}