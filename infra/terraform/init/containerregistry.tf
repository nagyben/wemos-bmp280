resource "google_artifact_registry_repository" "docker_repo_init" {
  provider = google-beta
  repository_id = "nagyben"
  description = "Docker container registry"
  format = "DOCKER"
}

resource "google_artifact_registry_repository_iam_member" "ci" {
  provider = google-beta

  location = google_artifact_registry_repository.docker_repo_init.location
  repository = google_artifact_registry_repository.docker_repo_init.name
  role   = "roles/artifactregistry.reader"
  member = "serviceAccount:${var.ci_sa}"
}