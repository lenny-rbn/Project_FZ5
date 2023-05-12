using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.InputSystem;

public class Player : MonoBehaviour
{
    [Header("Movement")]
    [SerializeField] private float maxSpeed = 10f;
    [SerializeField] private float acceleration = 20f;
    [SerializeField] private float deceleration = 2f;

    [Header("Jump")]
    [SerializeField] private int jumpVelocity = 10;
    [SerializeField] private float lowJumpMult = 3f;
    [SerializeField] private float fallMult = 1.5f;

    [Header("Dash")]
    [SerializeField] private float dashTime = 0.1f;
    [SerializeField] private float dashDistance = 1.5f;
    [SerializeField] private float dashCoolDown = 0.2f;
    [SerializeField] private float coefDecelDash = 1.75f;

    private bool isJumping;
    private bool isDashing;
    private bool isGrounded;

    private float rotation;
    private float lerpDash;
    private float dashTimer;

    private Vector3 move;

    Rigidbody player;

    private PlayerInput PlayerInput;
    private InputAction _move;
    private InputAction _jump;
    private InputAction _dash;

    void Start()
    {
        player = GetComponent<Rigidbody>();
        PlayerInput = GetComponent<PlayerInput>();

        lerpDash = 0f;
        dashTimer = -1f;

        #region Input Actions
        _move = PlayerInput.actions.FindAction("Move");
        _move.performed += ctx => move = ctx.ReadValue<Vector2>();
        _move.canceled += _ => move = Vector2.zero;

        _jump = PlayerInput.actions.FindAction("Jump");
        _jump.performed += _ => Jump();
        _jump.canceled += _ => isJumping = true;

        _dash = PlayerInput.actions.FindAction("Dash");
        _dash.performed += _ => Dash();
        #endregion
    }


    void Update()
    {
        isGrounded = IsGrounded();

        if (dashTimer > 0)
            dashTimer -= Time.deltaTime;
        else
            isDashing = false;
    }

    void FixedUpdate()
    {
        Move();

        // Jump gravity
        if (player.velocity.y > 0)
            player.velocity += fallMult * Physics.gravity.y * Time.deltaTime * Vector3.up;
        else if (player.velocity.y < 0 || isJumping)
            player.velocity += lowJumpMult * Physics.gravity.y * Time.deltaTime * Vector3.up;
    }


    private bool IsGrounded()
    {
        LayerMask mask = (1 << 8);
        mask = ~mask;
        isGrounded = false;

        if (Physics.Raycast(transform.position + Vector3.right * (-0.2f), Vector3.down, 1.2f, mask) || Physics.Raycast(transform.position + Vector3.right * (0.2f), Vector3.down, 1.2f, mask) || Physics.Raycast(transform.position, Vector3.down, 1.2f, mask) || Physics.Raycast(transform.position + Vector3.forward * (-0.2f), Vector3.down, 1.2f, mask) || Physics.Raycast(transform.position + Vector3.forward * (0.2f), Vector3.down, 1.2f, mask))
        {
            isGrounded = true;
            isJumping = false;
        }

        return isGrounded;
    }

    public void Jump()
    {
        if (isGrounded)
            player.velocity = Vector3.up * jumpVelocity;
    }

    public void Dash()
    {
        if (!isDashing && move != Vector3.zero)
        {
            isDashing = true;
            player.velocity = new Vector3(Mathf.Cos(rotation * Mathf.Deg2Rad) * dashDistance / dashTime, player.velocity.y, Mathf.Sin(rotation * Mathf.Deg2Rad) * dashDistance / dashTime);
            dashTimer = dashCoolDown;
            lerpDash = 1;
        }
    }

    public void Move()
    {
        if (move != Vector3.zero)
        {
            // Rotation
            rotation = Vector3.Angle(Vector3.right, move);
            if (move.y < 0)
                rotation *= -1;

            // Acceleration
            player.velocity += new Vector3(Mathf.Cos(rotation * Mathf.Deg2Rad) * acceleration, 0, Mathf.Sin(rotation * Mathf.Deg2Rad) * acceleration);

            // Velocity cap
            if (isDashing)
            {
                player.velocity = new Vector3(Mathf.Cos(rotation * Mathf.Deg2Rad) * (dashDistance / dashTime), player.velocity.y, Mathf.Sin(rotation * Mathf.Deg2Rad) * (dashDistance / dashTime));
                lerpDash -= Time.deltaTime * coefDecelDash;
            }
            else if (player.velocity.magnitude > maxSpeed)
                player.velocity = new Vector3(Mathf.Cos(rotation * Mathf.Deg2Rad) * maxSpeed, player.velocity.y, Mathf.Sin(rotation * Mathf.Deg2Rad) * maxSpeed);
        }
        else
        {
            if (isDashing)
            {
                player.velocity = new Vector3(Mathf.Cos(rotation * Mathf.Deg2Rad) * (dashDistance / dashTime), player.velocity.y, Mathf.Sin(rotation * Mathf.Deg2Rad) * (dashDistance / dashTime));
                lerpDash -= Time.deltaTime * coefDecelDash;
            }
            else
                player.velocity = new Vector3(0f, player.velocity.y, 0f);
        }
    }
}